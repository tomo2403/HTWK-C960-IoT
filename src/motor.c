//
// Created by tinkor on 03.09.25.
//
#include <math.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"

#include "motor.h"

#define MTR2_PIN1_GPIO GPIO_NUM_0
#define MTR2_PIN2_GPIO GPIO_NUM_1
#define MTR1_PWM_GPIO GPIO_NUM_2
#define MTR2_PWM_GPIO GPIO_NUM_3
#define MTR1_PIN1_GPIO GPIO_NUM_4
#define MTR1_PIN2_GPIO GPIO_NUM_5



void motor_init(void) {
    const gpio_config_t gpioConfig = {
        .pin_bit_mask = 1 << MTR1_PIN1_GPIO | 1 << MTR1_PIN2_GPIO | 1 << MTR2_PIN1_GPIO | 1 << MTR2_PIN2_GPIO,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&gpioConfig);

    const ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 1000,  // Set output frequency at 50 Hz; 20 ms period length
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    const ledc_channel_config_t ledc_channel = {
        .gpio_num       = MTR1_PWM_GPIO,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0, // Set duty to 50% // mid position
        .hpoint         = 0,
        //.flags.output_invert = 1
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    const ledc_channel_config_t ledc_channel1 = {
        .gpio_num       = MTR2_PWM_GPIO,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_1,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0,
        .hpoint         = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel1));
}

void set_motor1(const int8_t percentage) {
    if (percentage > 0) {
        gpio_set_level(MTR1_PIN1_GPIO, 0);
        gpio_set_level(MTR1_PIN2_GPIO, 1);
    } else {
        gpio_set_level(MTR1_PIN2_GPIO, 0);
        gpio_set_level(MTR1_PIN1_GPIO, 1);
    }

    const uint32_t duty = abs(percentage) * 5 + 500;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}


void set_motor2(const int8_t percentage) {
    if (percentage > 0) {
        gpio_set_level(MTR2_PIN1_GPIO, 1);
        gpio_set_level(MTR2_PIN2_GPIO, 0);
    } else {
        gpio_set_level(MTR2_PIN2_GPIO, 1);
        gpio_set_level(MTR2_PIN1_GPIO, 0);
        }

    const uint32_t duty = abs(percentage) * 5 + 500;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
}

void stop_motor1() {

    gpio_set_level(MTR2_PIN1_GPIO, 1);// gpios = 1 oder duty = 1023 funktioniert beides einzeln
    gpio_set_level(MTR2_PIN2_GPIO, 1);  // duty auf 0 wegen stromverbrauch?
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}
// noch als struct motor 1 und 2!
void stop_motor2() {
    gpio_set_level(MTR2_PIN1_GPIO, 1);
    gpio_set_level(MTR2_PIN2_GPIO, 1);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
}
