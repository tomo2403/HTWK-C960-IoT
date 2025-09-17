#ifndef HTWK_C960_IOT_JOYSTICK_H
#define HTWK_C960_IOT_JOYSTICK_H

#include <esp_log.h>
#include "driver/adc.h"
#include "driver/gpio.h"

// --- Joystick/Role Config ---
#define JS_ADC_WIDTH          ADC_WIDTH_BIT_12   // 0..4095
#define JS_ADC_ATTEN          ADC_ATTEN_DB_11    // voller Bereich bis ~3.3V
// Passe die Kanäle an deine Pins an (z. B. VRx -> GPIO34 = ADC1_CH6, VRy -> GPIO35 = ADC1_CH7)
#define JS_AXIS_X_CH          ADC1_CHANNEL_2     // GPIO2
#define JS_AXIS_Y_CH          ADC1_CHANNEL_3     // GPIO3
#define JS_BTN_GPIO           GPIO_NUM_11        // Joystick SW; GND beim Drücken
#define JS_BTN_ACTIVE_LEVEL   0

#define JS_SAMPLE_INTERVAL_MS 50
#define JS_DEADZONE_PC        10     // Prozent rund um die Mitte
#define JS_ACTIVITY_THRESHOLD 8      // Änderung in %-Punkten, die als "Bewegung" gilt
#define ROLE_DECISION_MS      5000   // Wie lange nach Boot auf Aktivität warten
#define JS_CALIB_MS           800    // Zeitfenster zur Mittelwert-Kalibrierung
#define JS_CALIB_SWEEP_MS     5000   // Zusätzliche Zeit zum Erfassen von min/max der Achsen
#define JS_ADAPT_EPS          2      // Rohwert-Differenz, ab der min/max adaptiv erweitert werden
#define LED_CALIB_SWEEP		  0		 // Pin für die LED, die zum Callibrieren leuchtet
// --- Befehlsprotokoll ---
#define CMD_PROTO_VER 1
#define CMD_MAGIC0    'C'
#define CMD_MAGIC1    'M'

typedef enum : uint8_t
{
	CMD_JOYSTICK = 1
} cmd_type_t;

// Neue Kalibrierstruktur je Achse
typedef struct
{
	int min;
	int mid;
	int max;
	bool inited;
} axis_calib_t;

typedef struct __attribute__((packed))
{
	uint8_t magic[2]; // 'C','M'
	uint8_t type; // cmd_type_t
	uint8_t ver; // CMD_PROTO_VER
} cmd_hdr_t;

typedef struct __attribute__((packed))
{
	cmd_hdr_t hdr;
	int8_t x_pct; // -100..+100 (links/rechts)
	int8_t y_pct; // -100..+100 (vor/zurück)
	uint8_t buttons; // Bit0: SW (gedrückt)
} cmd_joystick_t;

typedef enum
{
	ROLE_UNKNOWN = 0,
	ROLE_CONTROLLER,
	ROLE_CAR
} role_t;

static role_t s_role = ROLE_UNKNOWN;

void joystick_init(void);

int read_raw_axis(adc1_channel_t ch);

int8_t normalize_axis_with_cal(int raw, const axis_calib_t *c);

void adapt_axis_calib(axis_calib_t *c, int raw);

int8_t normalize_axis_to_pct_cal(int raw, int mid);

bool read_button_pressed(void);

void apply_motor_command_log(int8_t x_pct, int8_t y_pct, bool btn);

void led_calib_init();

void led_calib_toggle();

#endif //HTWK_C960_IOT_JOYSTICK_H
