//
// Created by tinkor on 15.09.25.
//

#ifndef LED_CONFIG_H
#define LED_CONFIG_H

typedef void (*KeyCallback)(uint8_t key);

void registerKeyCallback(KeyCallback keyCallback);

void keyCallback(uint8_t key);

void button_led_init();

void setup_hw_timer_led();
#endif //LED_CONFIG_H
