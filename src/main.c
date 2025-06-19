#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define IN1 GPIO_NUM_1
#define IN2 GPIO_NUM_2
#define IN3 GPIO_NUM_42
#define IN4 GPIO_NUM_41

void app_main(void)
{
    // Pins als Output setzen
    gpio_set_direction(IN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN3, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN4, GPIO_MODE_OUTPUT);

    while (1)
    {
        gpio_set_level(IN1, 1);
    }

    // while (1)
    // {
    //     // Motor A im Uhrzeigersinn
    //     gpio_set_level(IN1, 1);
    //     gpio_set_level(IN2, 0);
    //     vTaskDelay(pdMS_TO_TICKS(2000));
    //
    //     gpio_set_level(IN1, 1);
    //     gpio_set_level(IN2, 1);
    //     vTaskDelay(pdMS_TO_TICKS(500));
    //
    //     // Motor B im Uhrzeigersinn
    //     gpio_set_level(IN3, 1);
    //     gpio_set_level(IN4, 0);
    //     vTaskDelay(pdMS_TO_TICKS(2000));
    //
    //     gpio_set_level(IN3, 1);
    //     gpio_set_level(IN4, 1);
    //     vTaskDelay(pdMS_TO_TICKS(500));
    //
    //     // Motor A gegen Uhrzeigersinn
    //     gpio_set_level(IN1, 0);
    //     gpio_set_level(IN2, 1);
    //     vTaskDelay(pdMS_TO_TICKS(2000));
    //
    //     gpio_set_level(IN1, 1);
    //     gpio_set_level(IN2, 1);
    //     vTaskDelay(pdMS_TO_TICKS(500));
    //
    //     // Motor B gegen Uhrzeigersinn
    //     gpio_set_level(IN3, 0);
    //     gpio_set_level(IN4, 1);
    //     vTaskDelay(pdMS_TO_TICKS(2000));
    //
    //     gpio_set_level(IN3, 1);
    //     gpio_set_level(IN4, 1);
    //     vTaskDelay(pdMS_TO_TICKS(500));
    // }
}
