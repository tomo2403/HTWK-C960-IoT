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

#define DISABLE_HUMIDITY true

static const char *TAG = "AppManager";

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

    ESP_LOGI(TAG, "Starte MQTT");
    mqtt_app_start();

    ESP_LOGI(TAG, "Konfiguriere I2C");
    i2c_master_driver_initialize();

    ESP_LOGI(TAG, "Starte Sensoren");
    sensor_bmx280_init();
    sensor_sgp30_init();

    ESP_LOGI(TAG, "Warte auf MQTT Verbindung");
    mqtt_wait_connected(portMAX_DELAY);

    ESP_LOGI(TAG, "Starte Sensor Task");
    xTaskCreate(postSensorData, "sensor_task", 1024 * 2, 0, 10, NULL);
}
