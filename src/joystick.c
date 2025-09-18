#include "joystick.h"


// Hilfsfunktionen Joystick
void joystick_init(void) {

    adc1_config_width(JS_ADC_WIDTH);
    adc1_config_channel_atten(JS_AXIS_X_CH, JS_ADC_ATTEN);
    adc1_config_channel_atten(JS_AXIS_Y_CH, JS_ADC_ATTEN);

    const gpio_config_t io = {
        .pin_bit_mask = 1ULL << JS_BTN_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
}

int read_raw_axis(adc1_channel_t ch) {
    return adc1_get_raw(ch); // 0..4095
}

int8_t normalize_axis_with_cal(int raw, const axis_calib_t* c) {
    if (!c || !c->inited) return 0;

    // Schutz vor degenerate ranges
    const int pos_span = (c->max > c->mid) ? (c->max - c->mid) : 1;
    const int neg_span = (c->mid > c->min) ? (c->mid - c->min) : 1;

    float pct;
    if (raw >= c->mid) {
        pct = ((float)(raw - c->mid) / (float)pos_span) * 100.0f;
    } else {
        pct = -((float)(c->mid - raw) / (float)neg_span) * 100.0f;
    }

    // Deadzone um 0
    if (pct > -JS_DEADZONE_PC && pct < JS_DEADZONE_PC) pct = 0.0f;

    if (pct > 100.0f) pct = 100.0f;
    if (pct < -100.0f) pct = -100.0f;
    return (int8_t)pct;
}

// Optional: leichte adaptive Erweiterung, falls neue Extrema erreicht werden
void adapt_axis_calib(axis_calib_t* c, int raw) {
    if (!c || !c->inited) return;
    if (raw > c->max + JS_ADAPT_EPS) c->max = raw;
    if (raw < c->min - JS_ADAPT_EPS) c->min = raw;
}

// Kalibrierte Normalisierung: mappe Rohwert relativ zu kalibriertem Mittelpunkt auf -100..100
int8_t normalize_axis_to_pct_cal(int raw, int mid) {
    const int max = 4095;
    // Asymmetrische Spannen links/rechts um Mittelwert
    const float span_pos = (float)(max - mid);
    const float span_neg = (float)(mid - 0);
    float pct;

    if (raw >= mid) {
        pct = (span_pos > 1.0f) ? ((float)(raw - mid) / span_pos) * 100.0f : 0.0f;
    } else {
        pct = (span_neg > 1.0f) ? ((float)(raw - mid) / span_neg) * 100.0f : 0.0f;
    }

    // Deadzone um 0
    if (pct > -JS_DEADZONE_PC && pct < JS_DEADZONE_PC) pct = 0.0f;

    if (pct > 100.0f) pct = 100.0f;
    if (pct < -100.0f) pct = -100.0f;
    return (int8_t)pct;
}

bool read_button_pressed(void) {
    int level = gpio_get_level(JS_BTN_GPIO);
    return (level == JS_BTN_ACTIVE_LEVEL);
}

void apply_motor_command_log(int8_t x_pct, int8_t y_pct, bool btn) {

    set_motor1(x_pct);
    set_motor2(y_pct);
    // Platzhalter-Endpunkt: später Motoren ansteuern
    // y_pct > 0 vorwärts, < 0 rückwärts; x_pct < 0 links, > 0 rechts
    ESP_LOGI("MOTOR", "Cmd: steer=%d%%, throttle=%d%%, btn=%d", (int)x_pct, (int)y_pct, (int)btn);
}

void led_calib_init() {
    gpio_config_t gpioConfigIn = {
        .pin_bit_mask = {1 << LED_CALIB_SWEEP},
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&gpioConfigIn);
}

void led_calib_toggle() {
    uint32_t level= gpio_get_level(LED_CALIB_SWEEP);
    gpio_set_level(LED_CALIB_SWEEP, !level);
}