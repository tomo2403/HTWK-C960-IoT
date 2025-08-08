#ifndef WLAN_H
#define WLAN_H

#include "esp_event.h"

void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

void initStaEventHandlers();

void initWPA2Enterprise();

void initWPA2Personal();

void initSTA();

bool waitForSTAConnected(TickType_t timeout);

#endif //WLAN_H
