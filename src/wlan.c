#include "../include/wlan.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_eap_client.h"
#include <string.h>

#include "../include/secrets.h"

#define USE_WPA2_ENTERPRISE 0

EventGroupHandle_t wifi_event_group = NULL;
const int WIFI_CONNECTED_BIT = BIT0;


void eventHandler(void* arg, const esp_event_base_t event_base, const int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW("WifiManager", "Verbindung verloren, versuche erneut...");
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("WifiManager", "Verbunden, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void initStaEventHandlers()
{
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler, NULL, NULL);
}

void initWPA2Enterprise()
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WPA2E_SSID,
        },
    };
    memcpy(wifi_config.sta.ssid, CONFIG_WPA2E_SSID, sizeof(CONFIG_WPA2E_SSID));
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
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
}

void initSTA()
{
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

#if USE_WPA2_ENTERPRISE
    initWPA2Enterprise();
#else
    initWPA2Personal();
#endif

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}
