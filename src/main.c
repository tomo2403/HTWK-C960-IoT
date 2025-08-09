#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>

#include "mqtt.h"
#include "wlan.h"
#include "sensors.h"

static const char *TAG = "AppManager";

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(200)); // kurze Wartezeit

    // NVS robust initialisieren
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_LOGI(TAG, "Konfiguriere WiFi");
    initSTA();

    // TODO: MQTT
    ESP_LOGI(TAG, "Warte auf WiFi Verbindung");
    if (!waitForSTAConnected(portMAX_DELAY))
    {
        ESP_LOGE(TAG, "Konnte keine WiFi-Verbindung herstellen");
        return;
    }

    ESP_LOGI(TAG, "Starte MQTT");
    mqtt_app_start();

    // TODO: SENSORS
    ESP_LOGI(TAG, "Starte Sensoren");
    sensor_init();

    const uint16_t tvoc = sgp30->TVOC;
    char tvoc_str[10];
    snprintf(tvoc_str, sizeof(tvoc_str), "%u", tvoc);

    ESP_LOGI(TAG, "Warte auf MQTT Verbindung");
    mqtt_wait_connected(portMAX_DELAY);

    ESP_LOGI(TAG, "Sende Daten");
    mqtt_enqueue("/sensor/tvoc", tvoc_str, 0, 1, 0);
}
