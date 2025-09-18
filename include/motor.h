#ifndef MOTOR_H
#define MOTOR_H

#include <math.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"

#define MTR2_PIN1_GPIO GPIO_NUM_0
#define MTR2_PIN2_GPIO GPIO_NUM_1
#define MTR1_PWM_GPIO GPIO_NUM_2
#define MTR2_PWM_GPIO GPIO_NUM_3
#define MTR1_PIN1_GPIO GPIO_NUM_4
#define MTR1_PIN2_GPIO GPIO_NUM_5

void motor_init(void);

void set_motor1(int8_t percentage);

void set_motor2(int8_t percentage);

void stop_motor1();

void stop_motor2();


#endif //MOTOR_H
