#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>

#include "mqtt.h"
#include "wlan.h"
#include "sensors.h"

static const char *TAG = "wifi_sta";

void app_main(void)
{
	vTaskDelay(1000 / portTICK_PERIOD_MS); // Wartezeit für NVS-Initialisierung
	nvs_flash_init();

	initStaEventHandlers();
	initSTA();

	// TODO: MQTT
	mqtt_app_start();

	// TODO: SENSORS
	sensor_init();

	const uint16_t tvoc = sgp30->TVOC; // Assuming sgp30 is a struct instance
	char tvoc_str[10];
	snprintf(tvoc_str, sizeof(tvoc_str), "%u", tvoc);
	mqtt_enqueue("/sensor/tvoc", tvoc_str, 0, 1, 0);}
