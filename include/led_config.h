#ifndef LED_CONFIG_H
#define LED_CONFIG_H

#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_intr_alloc.h"
#include <esp_log.h>

#define BTN_GPIO GPIO_NUM_10
#define LED1_GPIO GPIO_NUM_18
#define LED2_GPIO GPIO_NUM_19

/**
 * Callback-Typ für Tastendrücke.
 *
 * Wird aus dem Button-Interrupt-Kontext aufgerufen. Die Implementierung sollte
 * daher schnell sein und idealerweise nur Ereignisse in eine Queue posten.
 *
 * @param key GPIO-Pin (z. B. BTN_GPIO), der das Ereignis ausgelöst hat.
 */
typedef void (*KeyCallback)(uint8_t key);

/**
 * Registriert eine Callback-Funktion für Button-Ereignisse.
 *
 * Der Callback wird aus der GPIO-ISR aufgerufen, wenn am BTN_GPIO ein
 * Flankenereignis erkannt wird. Übergib NULL, um den Callback zu entfernen.
 *
 * Threading/ISR: Wird aus Interrupt-Kontext getriggert; halte Verarbeitung kurz.
 *
 * @param keyCallback Funktionszeiger auf den Benutzer-Callback (oder NULL).
 */
void registerKeyCallback(KeyCallback keyCallback);

/**
 * Standard-Callback für Button-Ereignisse.
 *
 * Beispielimplementierung, die ein Blink-Flag toggelt und bei Deaktivierung
 * beide LEDs ausschaltet. Kann als Default an registerKeyCallback() übergeben
 * oder als Referenz für eine eigene Implementierung genutzt werden.
 *
 * @param key GPIO-Pin, der das Ereignis ausgelöst hat.
 */
void keyCallback(uint8_t key);

/**
 * Initialisiert Button- und LED-GPIOs.
 *
 * - BTN_GPIO wird als Eingang mit Pull-up und Negativflanken-Interrupt
 *   konfiguriert und im ISR-Service registriert.
 * - LED1_GPIO und LED2_GPIO werden als Output konfiguriert.
 *
 * Muss vor setup_hw_timer_led() aufgerufen werden, wenn LEDs blinken sollen.
 */
void button_led_init();

/**
 * Konfiguriert und startet einen HW-Timer für das Blaulicht-Blinkmuster.
 *
 * - Setzt einen periodischen Alarm (z. B. 500 ms).
 * - Registriert eine ISR, die LED1/LED2 wechselseitig toggelt, sofern
 *   das Blink-Flag (durch keyCallback toggelbar) aktiv ist.
 *
 * Voraussetzungen: Aufruf nach button_led_init().
 */
void setup_hw_timer_led();

#endif //LED_CONFIG_H
