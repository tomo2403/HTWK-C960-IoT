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

/**
 * Initialisiert die Motorsteuerung.
 *
 * - Konfiguriert die benötigten GPIOs als Ausgänge (Richtungs-Pins).
 * - Initialisiert den LEDC-PWM-Timer und die PWM-Kanäle für beide Motoren.
 * Muss vor set_motor1/set_motor2 aufgerufen werden.
 */
void motor_init(void);

/**
 * Setzt Geschwindigkeit und Richtung für Motor 1 anhand eines Prozentwerts.
 *
 * Mapping:
 * - percentage > 0: Vorwärtsfahrt
 * - percentage < 0: Rückwärtsfahrt
 * - percentage == 0: Motor wird gestoppt (stop_motor1)
 *
 * @param percentage Fahrbefehl in Prozent [-100..100].
 */
void set_motor1(int8_t percentage);

/**
 * Setzt Geschwindigkeit und Richtung für Motor 2 anhand eines Prozentwerts.
 *
 * Mapping:
 * - percentage > 0: Vorwärtsfahrt
 * - percentage < 0: Rückwärtsfahrt
 * - percentage == 0: Motor wird gestoppt (stop_motor2)
 *
 * @param percentage Fahrbefehl in Prozent [-100..100].
 */
void set_motor2(int8_t percentage);

/**
 * Stoppt Motor 1.
 *
 * Setzt die Treiber-Pins in einen Leerlauf-/Bremszustand und PWM-Duty auf 0.
 */
void stop_motor1();

/**
 * Stoppt Motor 2.
 *
 * Setzt die Treiber-Pins in einen Leerlauf-/Bremszustand und PWM-Duty auf 0.
 */
void stop_motor2();

#endif //MOTOR_H
