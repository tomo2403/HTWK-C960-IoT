#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#define IN1_GPIO        1   // PWM-Pin
#define IN2_GPIO        2   // Richtungspin
#define PWM_FREQ_HZ     1000
#define PWM_RES_BITS    LEDC_TIMER_10_BIT
#define PWM_DUTY        512  // 50% bei 10 Bit Auflösung (0–1023)

#define PWM_TIMER       LEDC_TIMER_0
#define PWM_CHANNEL   LEDC_CHANNEL_0
#define PWM_CHANNEL_2   LEDC_CHANNEL_2
#define PWM_MODE        LEDC_LOW_SPEED_MODE

void lenkung_stop() {
    // Beide Richtungen aus
    gpio_set_level(IN1_GPIO, 0);
    gpio_set_level(IN2_GPIO, 0);
    ledc_set_duty(PWM_MODE, PWM_CHANNEL, 0);
    ledc_update_duty(PWM_MODE, PWM_CHANNEL);
}

void lenkung_rechts() {
    gpio_set_level(IN2_GPIO, 1);
    ledc_set_duty(PWM_MODE, PWM_CHANNEL, PWM_DUTY);
    ledc_update_duty(PWM_MODE, PWM_CHANNEL);
}

void lenkung_links() {
    gpio_set_level(IN1_GPIO, 1);
    ledc_set_duty(PWM_MODE, PWM_CHANNEL_2, PWM_DUTY);
    ledc_update_duty(PWM_MODE, PWM_CHANNEL_2);
}

void app_main(void)
{
    // GPIOs für Richtung konfigurieren
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << IN1_GPIO) | (1ULL << IN2_GPIO),
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&io_conf);

    // PWM-Timer konfigurieren
    ledc_timer_config_t pwm_timer = {
        .speed_mode       = PWM_MODE,
        .duty_resolution  = PWM_RES_BITS,
        .timer_num        = PWM_TIMER,
        .freq_hz          = PWM_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&pwm_timer);

    // PWM-Kanal konfigurieren
    ledc_channel_config_t pwm_channel_pin1 = {
        .gpio_num       = IN1_GPIO,
        .speed_mode     = PWM_MODE,
        .channel        = PWM_CHANNEL,
        .timer_sel      = PWM_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };

    ledc_channel_config_t pwm_channel_pin2 = {
        .gpio_num       = IN2_GPIO,
        .speed_mode     = PWM_MODE,
        .channel        = PWM_CHANNEL_2,
        .timer_sel      = PWM_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };

    ledc_channel_config(&pwm_channel_pin1);
    //ledc_channel_config(&pwm_channel_pin2);

    while (1) {
        lenkung_rechts();
        vTaskDelay(pdMS_TO_TICKS(300));  // z.B. 300 ms nach rechts lenken
        lenkung_stop();
        vTaskDelay(pdMS_TO_TICKS(1000));

        lenkung_links();  // Achtung: nur digital HIGH auf IN2 → langsam
        vTaskDelay(pdMS_TO_TICKS(300));  // z.B. 300 ms nach links lenken
        lenkung_stop();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
