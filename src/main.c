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

static const char *TAG = "AppManager";

void postSensorData(void *args)
{
    while (1)
    {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        sgp30_IAQ_measure(&main_sgp30_sensor);
        ESP_LOGI(TAG, "TVOC: %d,  eCO2: %d", main_sgp30_sensor.TVOC, main_sgp30_sensor.eCO2);
        char tvoc_buf[12];
        char eco2_buf[12];
        const int tvoc_len = snprintf(tvoc_buf, sizeof(tvoc_buf), "%u", (unsigned)main_sgp30_sensor.TVOC);
        const int eco2_len = snprintf(eco2_buf, sizeof(eco2_buf), "%u", (unsigned)main_sgp30_sensor.eCO2);
        mqtt_enqueue("/sensor/tvoc", tvoc_buf, tvoc_len, 1, 0);
        mqtt_enqueue("/sensor/eco2", eco2_buf, eco2_len, 1, 0);
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
    if (!waitForSTAConnected(portMAX_DELAY))
    {
        ESP_LOGE(TAG, "Konnte keine WiFi-Verbindung herstellen");
        return;
    }

    ESP_LOGI(TAG, "Starte MQTT");
    mqtt_app_start();

    esp_log_level_set("SGP30-LIB", ESP_LOG_VERBOSE); // DEBUG
    sensor_sgp30_init();

    ESP_LOGI(TAG, "Warte auf MQTT Verbindung");
    mqtt_wait_connected(portMAX_DELAY);
    xTaskCreate(postSensorData, "sensor_task", 1024 * 2, (void *)0, 10, NULL);
}
