//
// ESPNOW bidirectional messaging with fragmentation and reassembly
//

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "../include/espnow.h"

#define TAG "espnow"

// Fragmentgröße: Header + Nutzlast <= 250 Bytes (ESPNOW Payload Limit)
// Wir wählen konservativ 200 Bytes Nutzlast.
#ifndef ESPNOW_FRAGMENT_PAYLOAD
#define ESPNOW_FRAGMENT_PAYLOAD 200
#endif

// Protokoll-Header für Fragmentierung
typedef struct __attribute__((packed)) {
    uint16_t msg_id;      // Nachrichten-ID (Sender-seitig vergeben)
    uint16_t seq_idx;     // 0..total_frags-1
    uint16_t total_frags; // Gesamtanzahl Fragmente
    uint16_t payload_len; // Länge der Nutzlast in diesem Fragment
} espnow_pkt_hdr_t;

typedef struct {
    bool used;
    uint8_t src_mac[6];
    uint16_t msg_id;
    uint16_t total_frags;
    uint16_t received_frags;
    size_t total_bytes;       // Summe payload_len aller Fragmente
    uint8_t* buffer;          // Zielspeicher für reassemblierten Payload
    size_t buffer_size;
    // Einfaches Bitmap, maximal 512 Fragmente (für ~100 kB bei 200B Payload)
    // Begrenzen wir auf 128 Fragmente (25.6 kB), um RAM zu schonen:
#ifndef ESPNOW_MAX_FRAGMENTS
#define ESPNOW_MAX_FRAGMENTS 128
#endif
    uint8_t frag_bitmap[(ESPNOW_MAX_FRAGMENTS + 7) / 8];
} espnow_reasm_t;

static struct {
    bool initialized;
    wifi_interface_t ifx;
    espnow_recv_cb_t recv_cb;
    void* user_ctx;
    SemaphoreHandle_t lock;
    uint16_t next_msg_id;

    // Reassembly-Slots
    espnow_reasm_t reasm[ESPNOW_MAX_INFLIGHT_MESSAGES];

} g_ctx = {0};

// Hilfsfunktionen
static inline void lock(void)   { if (g_ctx.lock) xSemaphoreTake(g_ctx.lock, portMAX_DELAY); }
static inline void unlock(void) { if (g_ctx.lock) xSemaphoreGive(g_ctx.lock); }

static bool mac_equal(const uint8_t a[6], const uint8_t b[6]) {
    return memcmp(a, b, 6) == 0;
}

static void set_bitmap(espnow_reasm_t* r, uint16_t idx) {
    r->frag_bitmap[idx / 8] |= (uint8_t)(1u << (idx % 8));
}
static bool get_bitmap(const espnow_reasm_t* r, uint16_t idx) {
    return (r->frag_bitmap[idx / 8] >> (idx % 8)) & 1u;
}

static espnow_reasm_t* reasm_find_or_alloc(const uint8_t src_mac[6], uint16_t msg_id, uint16_t total_frags) {
    // Suchen
    for (int i = 0; i < ESPNOW_MAX_INFLIGHT_MESSAGES; ++i) {
        if (g_ctx.reasm[i].used &&
            g_ctx.reasm[i].msg_id == msg_id &&
            mac_equal(g_ctx.reasm[i].src_mac, src_mac)) {
            return &g_ctx.reasm[i];
        }
    }
    // Allozieren
    for (int i = 0; i < ESPNOW_MAX_INFLIGHT_MESSAGES; ++i) {
        if (!g_ctx.reasm[i].used) {
            espnow_reasm_t* r = &g_ctx.reasm[i];
            memset(r, 0, sizeof(*r));
            r->used = true;
            memcpy(r->src_mac, src_mac, 6);
            r->msg_id = msg_id;
            r->total_frags = total_frags;
            return r;
        }
    }
    return NULL;
}

static void reasm_free(espnow_reasm_t* r) {
    if (!r) return;
    if (r->buffer) {
        free(r->buffer);
    }
    memset(r, 0, sizeof(*r));
}

static void ensure_buffer_for_fragment(espnow_reasm_t* r, uint16_t seq_idx, uint16_t payload_len) {
    // Wir schreiben Fragments payload direkt hintereinander in Reihenfolge seq_idx
    // Dafür benötigen wir total_frags * max_payload theoretisch. Wir gehen
    // dynamisch vor, indem wir bei erstem Fragment eine Obergrenze schätzen
    size_t min_size = (size_t)(seq_idx + 1) * ESPNOW_FRAGMENT_PAYLOAD;
    if (min_size > ESPNOW_MAX_MESSAGE_SIZE) min_size = ESPNOW_MAX_MESSAGE_SIZE;
    if (r->buffer == NULL) {
        size_t alloc = (size_t)r->total_frags * ESPNOW_FRAGMENT_PAYLOAD;
        if (alloc > ESPNOW_MAX_MESSAGE_SIZE) alloc = ESPNOW_MAX_MESSAGE_SIZE;
        r->buffer = (uint8_t*)malloc(alloc);
        r->buffer_size = r->buffer ? alloc : 0;
    }
    (void)payload_len;
}

static esp_err_t on_data_recv(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
    if (!recv_info || !data || len < (int)sizeof(espnow_pkt_hdr_t)) return ESP_OK;

    espnow_pkt_hdr_t hdr;
    memcpy(&hdr, data, sizeof(hdr));

    // Sanity checks
    if (hdr.total_frags == 0 || hdr.seq_idx >= hdr.total_frags || hdr.total_frags > ESPNOW_MAX_FRAGMENTS) {
        ESP_LOGW(TAG, "Invalid fragment header: msg=%u seq=%u total=%u", hdr.msg_id, hdr.seq_idx, hdr.total_frags);
        return ESP_OK;
    }
    if (hdr.payload_len + sizeof(hdr) != (uint16_t)len) {
        ESP_LOGW(TAG, "Length mismatch: hdr=%u actual=%d", hdr.payload_len, len);
        return ESP_OK;
    }

    const uint8_t* src_mac = recv_info->src_addr;
    const uint8_t* payload = data + sizeof(hdr);

    lock();
    espnow_reasm_t* r = reasm_find_or_alloc(src_mac, hdr.msg_id, hdr.total_frags);
    if (!r) {
        ESP_LOGW(TAG, "No reassembly slot available");
        unlock();
        return ESP_OK;
    }
    if (r->total_frags != hdr.total_frags) {
        // Konflikt — Nachricht neu beginnen
        reasm_free(r);
        r = reasm_find_or_alloc(src_mac, hdr.msg_id, hdr.total_frags);
        if (!r) { unlock(); return ESP_OK; }
    }

    ensure_buffer_for_fragment(r, hdr.seq_idx, hdr.payload_len);
    if (!r->buffer || r->buffer_size == 0) {
        ESP_LOGE(TAG, "Out of memory for reassembly");
        reasm_free(r);
        unlock();
        return ESP_OK;
    }

    if (!get_bitmap(r, hdr.seq_idx)) {
        // Kopiere an Position seq_idx * ESPNOW_FRAGMENT_PAYLOAD
        size_t offset = (size_t)hdr.seq_idx * (size_t)ESPNOW_FRAGMENT_PAYLOAD;
        if (offset + hdr.payload_len <= r->buffer_size) {
            memcpy(r->buffer + offset, payload, hdr.payload_len);
            r->received_frags++;
            r->total_bytes += hdr.payload_len;
            set_bitmap(r, hdr.seq_idx);
        } else {
            ESP_LOGW(TAG, "Fragment would overflow buffer");
        }
    }
    bool complete = (r->received_frags == r->total_frags);
    size_t total_bytes = r->total_bytes;
    uint8_t* full = NULL;

    if (complete) {
        full = (uint8_t*)malloc(total_bytes);
        if (full) {
            // Zusammensetzen in korrekter Reihenfolge
            size_t written = 0;
            for (uint16_t i = 0; i < r->total_frags; ++i) {
                if (!get_bitmap(r, i)) { written = 0; break; }
                size_t copy = ESPNOW_FRAGMENT_PAYLOAD;
                if (i == r->total_frags - 1) {
                    // Letztes Fragment kann kürzer sein: exakt rekonstruieren
                    size_t consumed = (size_t)(r->total_frags - 1) * ESPNOW_FRAGMENT_PAYLOAD;
                    copy = total_bytes - consumed;
                }
                size_t offset = (size_t)i * (size_t)ESPNOW_FRAGMENT_PAYLOAD;
                memcpy(full + written, r->buffer + offset, copy);
                written += copy;
            }
        } else {
            ESP_LOGE(TAG, "Out of memory for final assembly");
        }
        reasm_free(r);
    }
    unlock();

    if (complete && full && g_ctx.recv_cb) {
        g_ctx.recv_cb(src_mac, full, total_bytes, g_ctx.user_ctx);
    }
    if (full) free(full);
    return ESP_OK;
}

static void espnow_recv_cb(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
    (void)on_data_recv(recv_info, data, len);
}

static void espnow_send_cb(const wifi_tx_info_t* tx_info, esp_now_send_status_t status) {
    (void)tx_info; // Optional: Falls Sie nichts daraus lesen wollen
    ESP_LOGV(TAG, "Send status=%d", (int)status);
}

esp_err_t espnow_set_pmk(const uint8_t key[ESPNOW_KEY_LEN]) {
    if (!g_ctx.initialized) return ESP_ERR_INVALID_STATE;
    return esp_now_set_pmk(key);
}

esp_err_t espnow_get_our_mac(uint8_t mac_out[6]) {
    if (!mac_out) return ESP_ERR_INVALID_ARG;
    wifi_mode_t mode;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_get_mode(&mode)); // sollte bereits gesetzt sein
    esp_err_t err = esp_wifi_get_mac(g_ctx.ifx, mac_out);
    return err;
}

esp_err_t espnow_add_peer(const uint8_t peer_mac[6], const uint8_t* lmk, bool encrypt) {
    if (!g_ctx.initialized || !peer_mac) return ESP_ERR_INVALID_STATE;

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, peer_mac, 6);
    peer.ifidx = g_ctx.ifx;
    peer.channel = 0; // 0 = benutze aktuellen Kanal des Interfaces, koexistiert mit Wi-Fi
    peer.encrypt = encrypt;
    if (encrypt) {
        if (!lmk) return ESP_ERR_INVALID_ARG;
        memcpy(peer.lmk, lmk, ESPNOW_KEY_LEN);
    }
    // idf: esp_now_add_peer ist idempotent für existierenden Peer -> vorher entfernen zur Sicherheit
    esp_now_del_peer(peer.peer_addr);
    return esp_now_add_peer(&peer);
}

esp_err_t espnow_remove_peer(const uint8_t peer_mac[6]) {
    if (!g_ctx.initialized || !peer_mac) return ESP_ERR_INVALID_STATE;
    return esp_now_del_peer(peer_mac);
}

esp_err_t espnow_init(wifi_interface_t ifx, espnow_recv_cb_t recv_cb, void* user_ctx) {
    if (g_ctx.initialized) return ESP_OK;

    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.ifx = ifx;
    g_ctx.recv_cb = recv_cb;
    g_ctx.user_ctx = user_ctx;
    g_ctx.lock = xSemaphoreCreateMutex();
    if (!g_ctx.lock) return ESP_ERR_NO_MEM;

    // Wi-Fi muss bereits initialisiert und gestartet sein (STA/AP). Wir verändern das nicht.
    wifi_mode_t mode = WIFI_MODE_NULL;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_get_mode(&mode));
    if (mode == WIFI_MODE_NULL) {
        ESP_LOGE(TAG, "Wi-Fi is not initialized/started. Initialize Wi-Fi before espnow_init.");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = esp_now_init();
    if (err == ESP_ERR_ESPNOW_EXIST) err = ESP_OK;
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_now_init failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_now_register_send_cb(espnow_send_cb));

    // Standard-PMK kann optional gesetzt werden; ohne bleibt unverschlüsselt (per Peer steuerbar)
    // Beispiel: uint8_t pmk[16] = { ... }; espnow_set_pmk(pmk);

    g_ctx.initialized = true;
    ESP_LOGI(TAG, "ESPNOW initialized on ifx=%d", (int)ifx);
    return ESP_OK;
}

esp_err_t espnow_deinit(void) {
    if (!g_ctx.initialized) return ESP_OK;
    esp_now_unregister_recv_cb();
    esp_now_unregister_send_cb();
    esp_now_deinit();

    if (g_ctx.lock) {
        vSemaphoreDelete(g_ctx.lock);
        g_ctx.lock = NULL;
    }
    memset(&g_ctx, 0, sizeof(g_ctx));
    return ESP_OK;
}

static uint16_t next_msg_id(void) {
    lock();
    uint16_t id = ++g_ctx.next_msg_id;
    if (id == 0) id = ++g_ctx.next_msg_id; // 0 vermeiden
    unlock();
    return id;
}

esp_err_t espnow_send(const uint8_t peer_mac[6], const void* data, size_t len) {
    if (!g_ctx.initialized || !peer_mac || (!data && len > 0)) return ESP_ERR_INVALID_STATE;

    const uint8_t* p = (const uint8_t*)data;
    uint16_t total_frags = (len == 0) ? 1 : (uint16_t)((len + ESPNOW_FRAGMENT_PAYLOAD - 1) / ESPNOW_FRAGMENT_PAYLOAD);
    if (total_frags > ESPNOW_MAX_FRAGMENTS) {
        ESP_LOGE(TAG, "Message too large: requires %u fragments (max %u)", total_frags, (unsigned)ESPNOW_MAX_FRAGMENTS);
        return ESP_ERR_NO_MEM;
    }

    uint16_t msg_id = next_msg_id();

    for (uint16_t i = 0; i < total_frags; ++i) {
        size_t remaining = len - (size_t)i * ESPNOW_FRAGMENT_PAYLOAD;
        uint16_t chunk = (uint16_t)((remaining >= ESPNOW_FRAGMENT_PAYLOAD) ? ESPNOW_FRAGMENT_PAYLOAD : remaining);
        if (len == 0) chunk = 0;

        uint8_t frame[sizeof(espnow_pkt_hdr_t) + ESPNOW_FRAGMENT_PAYLOAD];
        espnow_pkt_hdr_t hdr = {
            .msg_id = msg_id,
            .seq_idx = i,
            .total_frags = total_frags,
            .payload_len = chunk
        };
        memcpy(frame, &hdr, sizeof(hdr));
        if (chunk > 0) {
            memcpy(frame + sizeof(hdr), p + (size_t)i * ESPNOW_FRAGMENT_PAYLOAD, chunk);
        }

        esp_err_t err = esp_now_send(peer_mac, frame, sizeof(hdr) + chunk);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_now_send failed at frag %u/%u: %s", i + 1, total_frags, esp_err_to_name(err));
            return err;
        }
        // Optional: kleine Verzögerung, um Congestion zu vermeiden
        // vTaskDelay(pdMS_TO_TICKS(1));
    }
    return ESP_OK;
}