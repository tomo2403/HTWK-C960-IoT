#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_intr_alloc.h"


void IRAM_ATTR timer_isr(void* arg) {
    static bool led_on = false;
    led_on = !led_on;
    gpio_set_level(6, led_on);
    gpio_set_level(7, !led_on);

    // Interrupt-Flag zurücksetzen und Alarm erneut aktivieren
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
}

void setup_hw_timer_led() {
    gpio_set_direction(6, GPIO_MODE_OUTPUT);
    gpio_set_direction(7, GPIO_MODE_OUTPUT);

    timer_config_t config = {
        .divider = 8000, // 80 MHz / 8000 = 10 kHz
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true,
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 5000); // 500ms (10kHz * 0.5s)
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, timer_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);
    timer_start(TIMER_GROUP_0, TIMER_0);
}