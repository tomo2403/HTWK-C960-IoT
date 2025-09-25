#ifndef WLAN_H
#define WLAN_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"

void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

void initStaEventHandlers(void);
void initWPA2Enterprise(void);
void initWPA2Personal(void);
void initSTA(void);

// wartet bis verbunden, true bei Erfolg
bool waitForSTAConnected(TickType_t timeout);

#endif // WLAN_H
