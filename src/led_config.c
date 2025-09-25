#include "led_config.h"

static void btn_isr_handler(void* arg);
static KeyCallback gKeyCallback = NULL;
volatile bool blink_enabled = false;

void btn_isr_handler(void* arg) {
    (void) arg;
    if (gKeyCallback != NULL) {
        gKeyCallback(BTN_GPIO);
    }
}

void keyCallback(uint8_t key) {
    blink_enabled = !blink_enabled;
    if (!blink_enabled) {
        gpio_set_level(LED1_GPIO, 0);
        gpio_set_level(LED2_GPIO, 0);
    }
}

void registerKeyCallback(KeyCallback keyCallback) {
    gKeyCallback = keyCallback;
}

void button_led_init() {

    //button setup with pullup and falling edge interrupt
    gpio_config_t gpioConfigIn = {
        .pin_bit_mask = {1 << BTN_GPIO},
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&gpioConfigIn);

    //
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_GPIO, btn_isr_handler, NULL);

    //led setup
    gpio_config_t gpioConfig = {
        .pin_bit_mask = {(1 << LED1_GPIO) | (1 << LED2_GPIO)},
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&gpioConfig);
}

// Timer Interrupt Service Routine - steuert Blaulicht
void IRAM_ATTR timer_isr(void* arg) {
    if (blink_enabled) {
        uint32_t ledLevel = gpio_get_level(LED1_GPIO);
        gpio_set_level(LED1_GPIO, !ledLevel);
        gpio_set_level(LED2_GPIO, ledLevel);
    }

    // Interrupt-Flag zurücksetzen und Alarm erneut aktivieren
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
}

// HW-Timer für LED-Blinken konfigurieren
void setup_hw_timer_led() {

    timer_config_t config = {
        .divider = 8000, // 80 MHz / 8000 = 10 kHz
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true,
    };

    int blinktime_interval_ms = 500;

    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, blinktime_interval_ms*10); // 500ms (10kHz * 0.5s)
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, timer_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);
    timer_start(TIMER_GROUP_0, TIMER_0);
}