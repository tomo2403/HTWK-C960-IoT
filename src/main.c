#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>
#include "../include/wlan.h"

static const char *TAG = "wifi_sta";

void app_main(void)
{
	vTaskDelay(1000 / portTICK_PERIOD_MS); // Wartezeit für NVS-Initialisierung
	nvs_flash_init();

	initStaEventHandlers();
	initSTA();

	// TODO: MQTT
}