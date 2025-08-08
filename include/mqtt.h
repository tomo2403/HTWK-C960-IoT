#ifndef MQTT_H
#define MQTT_H

#include "esp_event.h"
#include "mqtt_client.h"

extern esp_mqtt_client_handle_t client;

esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event);

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

void mqtt_app_start();

bool mqtt_is_connected(void);

bool mqtt_wait_connected(TickType_t timeout);

void mqtt_enqueue(const char *topic, const char *data, int len, int qos, int retain);

#endif //MQTT_H
