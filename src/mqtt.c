#include "mqtt.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "secrets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

esp_mqtt_client_handle_t client;
static EventGroupHandle_t s_mqtt_event_group;

#define MQTT_CONNECTED_BIT BIT0

static const char *TAG = "MqttManager";

esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    client = event->client;

    switch (event->event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Verbunden mit MQTT-Broker");
            if (s_mqtt_event_group) xEventGroupSetBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            esp_mqtt_client_publish(client, "/sensor/temp", "22.5", 0, 1, 0);
            esp_mqtt_client_publish(client, "/sensor/humidity", "60", 0, 1, 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT getrennt");
            if (s_mqtt_event_group) xEventGroupClearBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

bool mqtt_is_connected(void)
{
    if (!s_mqtt_event_group) return false;
    const EventBits_t bits = xEventGroupGetBits(s_mqtt_event_group);
    return (bits & MQTT_CONNECTED_BIT) != 0;
}

bool mqtt_wait_connected(TickType_t timeout)
{
    if (!s_mqtt_event_group) return false;
    const EventBits_t bits = xEventGroupWaitBits(
        s_mqtt_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdTRUE, timeout);
    return (bits & MQTT_CONNECTED_BIT) != 0;
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    mqtt_event_handler_cb(event_data);
}

void mqtt_app_start(void)
{
    if (!s_mqtt_event_group)
    {
        s_mqtt_event_group = xEventGroupCreate();
    }

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void mqtt_enqueue(const char *topic, const char *data, int len, int qos, int retain)
{
    esp_mqtt_client_enqueue(client, topic, data, len, qos, retain, true);
}
