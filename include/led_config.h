//
// Created by tinkor on 15.09.25.
//

#ifndef LED_CONFIG_H
#define LED_CONFIG_H

#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_intr_alloc.h"
#include <esp_log.h>

#define BTN_GPIO GPIO_NUM_10
#define LED1_GPIO GPIO_NUM_18
#define LED2_GPIO GPIO_NUM_19



typedef void (*KeyCallback)(uint8_t key);

void registerKeyCallback(KeyCallback keyCallback);

void keyCallback(uint8_t key);

void button_led_init();

void setup_hw_timer_led();
#endif //LED_CONFIG_H
