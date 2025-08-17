#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>
#include <stdio.h>

#include "mqtt.h"
#include "wlan.h"
#include "sensors.h"
#include "ntp.h"
#include "espnow.h"
#include "joystick.h"

#define DISABLE_HUMIDITY true

static const char *TAG = "AppManager";

// Gemeinsames Discovery-Token (an beiden Geräten identisch setzen!)
static const char DISCOVERY_TOKEN[] = "C960-ESPNOW-TOKEN";

// ESPNOW Broadcast-MAC
static const uint8_t ESPNOW_BCAST_MAC[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// Discovery-Frame-Typen
typedef enum : uint8_t {
    DISC_HELLO = 1,
    DISC_ACK   = 2
} disc_type_t;

typedef struct __attribute__((packed)) {
    uint8_t  type;  // DISC_HELLO oder DISC_ACK
    uint8_t  ver;   // Protokollversion
    uint16_t flags; // reserviert
    // Danach: Token als ASCII (ohne Nullterminator), direkt angehängt
    // Layout: [header][token bytes]
} disc_hdr_t;

#define DISC_PROTO_VER 1

// Lokale Peer-Verwaltung (klein & simpel)
typedef struct {
    uint8_t mac[6];
    bool used;
} peer_entry_t;


peer_entry_t s_known_peers[ESPNOW_MAX_PEERS] = {0};

// Hilfsfunktionen für Peerliste
bool mac_equal6(const uint8_t a[6], const uint8_t b[6]) {
    return memcmp(a, b, 6) == 0;
}
bool peer_known(const uint8_t mac[6]) {
    for (int i = 0; i < ESPNOW_MAX_PEERS; ++i) {
        if (s_known_peers[i].used && mac_equal6(s_known_peers[i].mac, mac)) return true;
    }
    return false;
}
bool peer_add_local(const uint8_t mac[6]) {
    if (peer_known(mac)) return true;
    for (int i = 0; i < ESPNOW_MAX_PEERS; ++i) {
        if (!s_known_peers[i].used) {
            memcpy(s_known_peers[i].mac, mac, 6);
            s_known_peers[i].used = true;
            return true;
        }
    }
    return false;
}

// ESPNOW-Empfangs-Callback: vollständige Nutzdaten
void on_espnow_recv(const uint8_t mac[6], const uint8_t* data, size_t len, void* user_ctx)
{
    (void)user_ctx;

    if (len >= sizeof(disc_hdr_t)) {
        const disc_hdr_t* hdr = (const disc_hdr_t*)data;
        const char* token_rx = (const char*)(data + sizeof(disc_hdr_t));
        size_t token_len = len - sizeof(disc_hdr_t);

        // Discovery-HELLO behandeln
        if (hdr->type == DISC_HELLO && hdr->ver == DISC_PROTO_VER) {
            if (token_len == strlen(DISCOVERY_TOKEN) &&
                memcmp(token_rx, DISCOVERY_TOKEN, token_len) == 0) {

                // Peer ggf. hinzufügen
                if (!peer_known(mac)) {
                    ESP_LOGI("ESPNOW", "Discovery: neuer Peer %02X:%02X:%02X:%02X:%02X:%02X",
                             mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
                    if (espnow_add_peer(mac, NULL, false) == ESP_OK && peer_add_local(mac)) {
                        ESP_LOGI("ESPNOW", "Peer hinzugefügt");
                    } else {
                        ESP_LOGW("ESPNOW", "Peer konnte nicht hinzugefügt werden (Liste voll?)");
                    }
                }

                // Unicast ACK zurücksenden
                uint8_t ack_buf[sizeof(disc_hdr_t) + sizeof(DISCOVERY_TOKEN) - 1];
                disc_hdr_t ack = {.type = DISC_ACK, .ver = DISC_PROTO_VER, .flags = 0};
                memcpy(ack_buf, &ack, sizeof(ack));
                memcpy(ack_buf + sizeof(ack), DISCOVERY_TOKEN, sizeof(DISCOVERY_TOKEN) - 1);
                espnow_send(mac, ack_buf, sizeof(ack_buf));
                return;
            }
        }

        // Discovery-ACK behandeln (nur Log/Bestätigung)
        if (hdr->type == DISC_ACK && hdr->ver == DISC_PROTO_VER) {
            if (token_len == strlen(DISCOVERY_TOKEN) &&
                memcmp(token_rx, DISCOVERY_TOKEN, token_len) == 0) {
                // Gegenstelle bestätigt – sicherstellen, dass sie als Peer erfasst ist
                if (!peer_known(mac)) {
                    if (espnow_add_peer(mac, NULL, false) == ESP_OK && peer_add_local(mac)) {
                        ESP_LOGI("ESPNOW", "Peer via ACK hinzugefügt: %02X:%02X:%02X:%02X:%02X:%02X",
                                 mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
                    }
                }
                ESP_LOGI("ESPNOW", "Discovery ACK von %02X:%02X:%02X:%02X:%02X:%02X",
                         mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
                return;
            }
        }
    }

    // Prüfe auf Steuerbefehle (CMD)
    if (len >= sizeof(cmd_hdr_t)) {
        const cmd_hdr_t* ch = (const cmd_hdr_t*)data;
        if (ch->magic[0] == CMD_MAGIC0 && ch->magic[1] == CMD_MAGIC1 &&
            ch->ver == CMD_PROTO_VER) {

            if (ch->type == CMD_JOYSTICK && len >= sizeof(cmd_joystick_t)) {
                const cmd_joystick_t* cj = (const cmd_joystick_t*)data;

                // Wenn wir keine Controller-Rolle haben, übernehme Auto-Rolle
                if (s_role != ROLE_CONTROLLER) {
                    if (s_role != ROLE_CAR) {
                        s_role = ROLE_CAR;
                        ESP_LOGI("ROLE", "Rolle -> CAR (Empfänger) nach Eingang von Befehlen");
                    }
                }

                const bool btn = (cj->buttons & 0x01) != 0;
                apply_motor_command_log(cj->x_pct, cj->y_pct, btn);
                return;
            }
        }
    }

    // Normale Nutzdaten
    ESP_LOGI("ESPNOW", "Empfangen: %u Bytes von %02X:%02X:%02X:%02X:%02X:%02X",
             (unsigned)len, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Periodisches Discovery-Beaconing (Broadcast HELLO)
void espnow_discovery_task(void* arg)
{
    (void)arg;
    // HELLO-Payload vorbereiten
    uint8_t hello_buf[sizeof(disc_hdr_t) + sizeof(DISCOVERY_TOKEN) - 1];
    disc_hdr_t hello = {.type = DISC_HELLO, .ver = DISC_PROTO_VER, .flags = 0};
    memcpy(hello_buf, &hello, sizeof(hello));
    memcpy(hello_buf + sizeof(hello), DISCOVERY_TOKEN, sizeof(DISCOVERY_TOKEN) - 1);

    for (;;) {
        // Broadcast-HELLO senden
        espnow_send(ESPNOW_BCAST_MAC, hello_buf, sizeof(hello_buf));
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// Kleines Test-Payload-Format
typedef struct __attribute__((packed)) {
    uint32_t counter;
    char     message[32]; // kurze Testnachricht
} test_payload_t;

// Joystick Sampling + Senden
void joystick_sender_task(void* arg) {
    (void)arg;
    joystick_init();

    // Phase 1: Mittelpunkt-Kalibrierung
    uint32_t start_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    uint32_t now_ms = start_ms;
    uint32_t cnt = 0;
    uint64_t sum_x = 0, sum_y = 0;

    while ((now_ms - start_ms) < JS_CALIB_MS) {
        sum_x += (uint32_t)read_raw_axis(JS_AXIS_X_CH);
        sum_y += (uint32_t)read_raw_axis(JS_AXIS_Y_CH);
        cnt++;
        vTaskDelay(pdMS_TO_TICKS(10));
        now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    }
    axis_calib_t cal_x = {0}, cal_y = {0};
    cal_x.mid = (cnt > 0) ? (int)(sum_x / cnt) : 2048;
    cal_y.mid = (cnt > 0) ? (int)(sum_y / cnt) : 2048;

    // Phase 2: Sweep für min/max (bitte in der Zeit einmal zu allen Extremen bewegen)
    start_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    now_ms = start_ms;
    cal_x.min = cal_x.max = cal_x.mid;
    cal_y.min = cal_y.max = cal_y.mid;

    while ((now_ms - start_ms) < JS_CALIB_SWEEP_MS) {
        const int rx = read_raw_axis(JS_AXIS_X_CH);
        const int ry = read_raw_axis(JS_AXIS_Y_CH);

        if (rx < cal_x.min) cal_x.min = rx;
        if (rx > cal_x.max) cal_x.max = rx;
        if (ry < cal_y.min) cal_y.min = ry;
        if (ry > cal_y.max) cal_y.max = ry;

        vTaskDelay(pdMS_TO_TICKS(10));
        now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    }

    // Fallbacks, falls keine Bewegung stattfand
    if (cal_x.min == cal_x.max) { cal_x.min = cal_x.mid - 100; cal_x.max = cal_x.mid + 100; }
    if (cal_y.min == cal_y.max) { cal_y.min = cal_y.mid - 100; cal_y.max = cal_y.mid + 100; }
    cal_x.inited = true;
    cal_y.inited = true;

    ESP_LOGI("JOYSTICK", "Cal X: min=%d mid=%d max=%d | Cal Y: min=%d mid=%d max=%d",
             cal_x.min, cal_x.mid, cal_x.max, cal_y.min, cal_y.mid, cal_y.max);

    int8_t last_x = 0, last_y = 0;
    bool last_btn = false;
    uint32_t t0 = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    bool activity_detected = false;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(JS_SAMPLE_INTERVAL_MS));

        const int raw_x = read_raw_axis(JS_AXIS_X_CH);
        const int raw_y = read_raw_axis(JS_AXIS_Y_CH);
        const bool btn = read_button_pressed();

        // Adaptiv Extrema leicht erweitern (optional)
        adapt_axis_calib(&cal_x, raw_x);
        adapt_axis_calib(&cal_y, raw_y);

        int8_t x_pct = normalize_axis_with_cal(raw_x, &cal_x);
        int8_t y_pct = normalize_axis_with_cal(raw_y, &cal_y);

        // Hinweis: Achsen bei Bedarf invertieren
        // x_pct = -x_pct; // falls rechts/links vertauscht
        // y_pct = -y_pct; // falls vor/zurück vertauscht

        // Aktivitätserkennung in der Anlaufphase
        uint32_t now_ms2 = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        if (s_role == ROLE_UNKNOWN && (now_ms2 - t0) <= ROLE_DECISION_MS) {
            if (abs(x_pct - last_x) >= JS_ACTIVITY_THRESHOLD ||
                abs(y_pct - last_y) >= JS_ACTIVITY_THRESHOLD ||
                btn != last_btn) {
                activity_detected = true;
            }
        } else if (s_role == ROLE_UNKNOWN) {
            s_role = activity_detected ? ROLE_CONTROLLER : ROLE_CAR;
            ESP_LOGI("ROLE", "Rolle entschieden: %s",
                     s_role == ROLE_CONTROLLER ? "CONTROLLER (Sender)" : "CAR (Empfänger)");
        }

        // Wenn Controller -> Befehle senden
        if (s_role == ROLE_CONTROLLER) {
            // Paket bauen
            cmd_joystick_t pkt = {0};
            pkt.hdr.magic[0] = CMD_MAGIC0;
            pkt.hdr.magic[1] = CMD_MAGIC1;
            pkt.hdr.type = CMD_JOYSTICK;
            pkt.hdr.ver = CMD_PROTO_VER;
            pkt.x_pct = x_pct;
            pkt.y_pct = y_pct;
            pkt.buttons = btn ? 0x01 : 0x00;

            // 1) Broadcast
            espnow_send(ESPNOW_BCAST_MAC, &pkt, sizeof(pkt));

            // 2) Unicast zu bekannten Peers
            for (int i = 0; i < ESPNOW_MAX_PEERS; ++i) {
                if (s_known_peers[i].used) {
                    esp_err_t err = espnow_send(s_known_peers[i].mac, &pkt, sizeof(pkt));
                    if (err != ESP_OK) {
                        ESP_LOGW("ESPNOW", "Cmd an %02X:%02X:%02X:%02X:%02X:%02X fehlgeschlagen: %s",
                                 s_known_peers[i].mac[0], s_known_peers[i].mac[1], s_known_peers[i].mac[2],
                                 s_known_peers[i].mac[3], s_known_peers[i].mac[4], s_known_peers[i].mac[5],
                                 esp_err_to_name(err));
                    }
                }
            }
        }

        last_x = x_pct;
        last_y = y_pct;
        last_btn = btn;
    }
}

// Sende-Task: schickt alle 3s ein Testpaket an alle bekannten Peers und einmal Broadcast
void espnow_test_sender_task(void* arg)
{
    (void)arg;
    uint32_t counter = 0;

    static const uint8_t BCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    while (1)
    {
        test_payload_t p = {0};
        p.counter = counter++;
        snprintf(p.message, sizeof(p.message), "hello-%lu", (unsigned long)p.counter);

        // 1) Broadcast (optional, zu Demo-Zwecken)
        espnow_send(BCAST, &p, sizeof(p));

        // 2) Unicast an alle bekannten Peers
        for (int i = 0; i < ESPNOW_MAX_PEERS; ++i) {
            if (s_known_peers[i].used) {
                esp_err_t err = espnow_send(s_known_peers[i].mac, &p, sizeof(p));
                if (err != ESP_OK) {
                    ESP_LOGW("ESPNOW", "Send an %02X:%02X:%02X:%02X:%02X:%02X fehlgeschlagen: %s",
                             s_known_peers[i].mac[0], s_known_peers[i].mac[1], s_known_peers[i].mac[2],
                             s_known_peers[i].mac[3], s_known_peers[i].mac[4], s_known_peers[i].mac[5],
                             esp_err_to_name(err));
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}


[[noreturn]]
void postSensorData(void *args)
{
    float temp = 0, pres = 0, hum = 0;
    char tvoc_buf[16], eco2_buf[16], temp_buf[16], pres_buf[16], hum_buf[16];
    while (1)
    {
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        // SGP30
        sgp30_IAQ_measure(&main_sgp30_sensor);

        // BME280
        bmx280_setMode(bmx280, BMX280_MODE_FORCE);
        while (bmx280_isSampling(bmx280))
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        ESP_ERROR_CHECK(bmx280_readoutFloat(bmx280, &temp, &pres, &hum));
        pres /= 100; // Convert to hPa

        const int tvoc_len = snprintf(tvoc_buf, sizeof(tvoc_buf), "%u", (unsigned) main_sgp30_sensor.TVOC);
        const int eco2_len = snprintf(eco2_buf, sizeof(eco2_buf), "%u", (unsigned) main_sgp30_sensor.eCO2);
        const int temp_len = snprintf(temp_buf, sizeof(temp_buf), "%.2f", temp);
        const int pres_len = snprintf(pres_buf, sizeof(pres_buf), "%.2f", pres);
#if !DISABLE_HUMIDITY
        const int hum_len = snprintf(hum_buf, sizeof(hum_buf), "%.2f", hum);
#endif

        mqtt_enqueue("/sensor/tvoc", tvoc_buf, tvoc_len, 1, 0);
        mqtt_enqueue("/sensor/eco2", eco2_buf, eco2_len, 1, 0);
        mqtt_enqueue("/sensor/temperature", temp_buf, temp_len, 1, 0);
        mqtt_enqueue("/sensor/pressure", pres_buf, pres_len, 1, 0);
#if !DISABLE_HUMIDITY
        mqtt_enqueue("/sensor/humidity", hum_buf, hum_len, 1, 0);
#endif

        ESP_LOGD(TAG, "TVOC: %s,  eCO2: %s, Temp: %s, Pres: %s, Hum: %s", tvoc_buf, eco2_buf, temp_buf, pres_buf, hum_buf);
    }
}

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(200));

    const esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_LOGI(TAG, "Konfiguriere WiFi");
    initSTA();

    ESP_LOGI(TAG, "Warte auf WiFi Verbindung");
    waitForSTAConnected(portMAX_DELAY);
    ntp_obtain_time();

    // ESPNOW initialisieren
    ESP_LOGI(TAG, "Initialisiere ESPNOW");
    ESP_ERROR_CHECK(espnow_init(WIFI_IF_STA, on_espnow_recv, NULL));

    // Optional: PMK setzen, falls Verschlüsselung gewünscht:
    // const uint8_t pmk[ESPNOW_KEY_LEN] = { /* 16 Bytes gemeinsamer Schlüssel */ };
    // ESP_ERROR_CHECK(espnow_set_pmk(pmk));

    // Broadcast-Peer registrieren (robust für Broadcast-Send)
    ESP_ERROR_CHECK(espnow_add_peer(ESPNOW_BCAST_MAC, NULL, false));

    // Eigene MAC loggen
    uint8_t our_mac[6] = {0};
    ESP_ERROR_CHECK(espnow_get_our_mac(our_mac));
    ESP_LOGI("ESPNOW", "Unsere MAC (STA): %02X:%02X:%02X:%02X:%02X:%02X",
             our_mac[0], our_mac[1], our_mac[2], our_mac[3], our_mac[4], our_mac[5]);

    // Discovery-Task starten
    xTaskCreate(espnow_discovery_task, "espnow_disc", 2048, NULL, 8, NULL);

    // Joystick + Rollenerkennung & Senden
    xTaskCreate(joystick_sender_task, "js_sender", 4096, NULL, 9, NULL);

    // ESP_LOGI(TAG, "Starte MQTT");
    // mqtt_app_start();
    //
    // ESP_LOGI(TAG, "Konfiguriere I2C");
    // i2c_master_driver_initialize();
    //
    // ESP_LOGI(TAG, "Starte Sensoren");
    // sensor_bmx280_init();
    // sensor_sgp30_init();
    //
    // ESP_LOGI(TAG, "Warte auf MQTT Verbindung");
    // mqtt_wait_connected(portMAX_DELAY);
    //
    // ESP_LOGI(TAG, "Starte Sensor Task");
    // xTaskCreate(postSensorData, "sensor_task", 1024 * 2, 0, 10, NULL);
}
