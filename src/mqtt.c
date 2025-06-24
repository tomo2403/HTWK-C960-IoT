#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_netif.h"

#define WIFI_SSID "tomo's ichTelefon"
#define WIFI_PASS "keifbr7374!kef"
#define MQTT_BROKER_URI "mqtt://172.20.10.3"

static const char *TAG = "MQTT_WIFI";

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static void event_handler(void* arg, esp_event_base_t event_base,
						  int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		ESP_LOGI(TAG, "Verbindung verloren, versuche erneut...");
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "Verbunden, IP: " IPSTR, IP2STR(&event->ip_info.ip));
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

void wifi_init_sta(void)
{
	wifi_event_group = xEventGroupCreate();

	esp_netif_init();
	esp_event_loop_create_default();
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);

	esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
	esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PASS,
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,
		},
	};

	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

	esp_wifi_start();
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
	esp_mqtt_client_handle_t client = event->client;
	if (event->event_id == MQTT_EVENT_CONNECTED) {
		ESP_LOGI(TAG, "MQTT verbunden");
		esp_mqtt_client_publish(client, "/sensor/temp", "22.5", 0, 1, 0);
		esp_mqtt_client_publish(client, "/sensor/humidity", "60", 0, 1, 0);
	}
	return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
	mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void) {
	const esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = MQTT_BROKER_URI,
	};

	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
	esp_mqtt_client_start(client);
}

void app_main(void) {
	nvs_flash_init();
	wifi_init_sta();
	vTaskDelay(pdMS_TO_TICKS(5000)); // warte auf WLAN-Verbindung
	mqtt_app_start();
}
