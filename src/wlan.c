#include "wlan.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_eap_client.h"
#include <string.h>

#include "secrets.h"

#define USE_WPA2_ENTERPRISE 0

static const char *TAG_WIFI = "WifiManager";

static EventGroupHandle_t wifi_event_group = NULL;
static const int WIFI_CONNECTED_BIT = BIT0;

void eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG_WIFI, "Event dispatched from event loop base=%s, event_id=%ld",
             event_base, (long)event_id);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG_WIFI, "Verbindung verloren, versuche erneut...");
        // TODO: Backoff/Timer einbauen statt sofort
        esp_wifi_connect();
        if (wifi_event_group) xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        const ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG_WIFI, "Verbunden, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        if (wifi_event_group) xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void initStaEventHandlers()
{
    // Die Default-Event-Loop MUSS vorher existieren, sonst schlägt die Registrierung fehl.
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler, NULL, NULL));
}

static void fill_sta_common(wifi_config_t *cfg)
{
    // Robustere Defaults für WPA3-Transition Netze
    cfg->sta.pmf_cfg.capable = true;
    cfg->sta.pmf_cfg.required = true; // WPA3 benötigt PMF
#if ESP_IDF_VERSION_MAJOR >= 5
    cfg->sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH; // Hash-to-Element + Hunting-and-Pecking
#endif
    // Optional: Channelscan/Listen-Interval Defaults können bei instabilen Netzen helfen
    cfg->sta.listen_interval = 0; // 0 = Default
}

void initWPA2Enterprise()
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WPA2E_SSID,
        },
    };
    memcpy(wifi_config.sta.ssid, CONFIG_WPA2E_SSID, sizeof(CONFIG_WPA2E_SSID));
    fill_sta_common(&wifi_config);
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_eap_client_set_identity((uint8_t *)CONFIG_WPA2E_IDENTITY, strlen(CONFIG_WPA2E_IDENTITY)));
    ESP_ERROR_CHECK(esp_eap_client_set_username((uint8_t *)CONFIG_WPA2E_USERNAME, strlen(CONFIG_WPA2E_USERNAME)));
    ESP_ERROR_CHECK(esp_eap_client_set_password((uint8_t *)CONFIG_WPA2E_PASSWORD, strlen(CONFIG_WPA2E_PASSWORD)));
    ESP_ERROR_CHECK(esp_wifi_sta_enterprise_enable());
}

void initWPA2Personal()
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WPA2_SSID,
            .password = CONFIG_WPA2_PASSWORD,
            // Threshold darf WPA3 nicht verhindern; WPA2 ist Mindestanforderung
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    fill_sta_common(&wifi_config);
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
}

void initSTA(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    initStaEventHandlers();

    const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Während Verbindungsaufbau PS deaktiviert (robuster bei WPA3/DHCP)
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

#if USE_WPA2_ENTERPRISE
    initWPA2Enterprise();
#else
    initWPA2Personal();
#endif

    ESP_ERROR_CHECK(esp_wifi_start());
}

bool waitForSTAConnected(const TickType_t timeout)
{
    if (!wifi_event_group) return false;
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    const EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, timeout);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}
