#ifndef MQTT_H
#define MQTT_H

#include "esp_event.h"
#include "mqtt_client.h"

/**
 * Globale Handle-Referenz auf den MQTT-Client.
 *
 * Gültig nach erfolgreichem Start in mqtt_app_start() und solange der
 * Client aktiv ist. Kann in seltenen Fällen während der (Re-)Initialisierung
 * kurzzeitig NULL sein.
 */
extern esp_mqtt_client_handle_t client;

/**
 * Low-Level MQTT-Event-Callback.
 *
 * Wird von mqtt_event_handler() aufgerufen und verarbeitet Kernereignisse
 * wie Verbindung/Trennung. Setzt/cleart intern ein EventBit zur
 * Verbindungsanzeige.
 *
 * Thread-Kontext: Wird aus dem ESP-Event-Loop aufgerufen.
 *
 * @param event Zeiger auf das MQTT-Event.
 * @return ESP_OK bei erfolgreicher Verarbeitung.
 */
esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event);

/**
 * Wrapper-Funktion, die beim Event-Loop registriert wird.
 *
 * Delegiert die eigentliche Verarbeitung an mqtt_event_handler_cb().
 *
 * @param handler_args Benutzerargumente (ungenutzt).
 * @param base         Event-Basis.
 * @param event_id     Event-ID.
 * @param event_data   Event-Daten (vom Typ esp_mqtt_event_handle_t).
 */
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

/**
 * Initialisiert und startet den MQTT-Client.
 *
 * - Erstellt eine Event-Gruppe zur Verbindungsüberwachung.
 * - Initialisiert den Client mit Konfiguration (z. B. Broker-URI).
 * - Registriert Event-Handler und startet die Verbindung.
 *
 * Idempotent: Mehrfache Aufrufe sind unkritisch.
 */
void mqtt_app_start();

/**
 * Prüft, ob aktuell eine Verbindung zum MQTT-Broker besteht.
 *
 * @return true, wenn verbunden; sonst false.
 */
bool mqtt_is_connected(void);

/**
 * Wartet blockierend auf eine aktive MQTT-Verbindung.
 *
 * @param timeout Maximale Wartezeit in OS-Ticks (portMAX_DELAY für unbegrenzt).
 * @return true, wenn innerhalb des Timeouts verbunden; sonst false.
 */
bool mqtt_wait_connected(TickType_t timeout);

/**
 * Enqueue eines Publishs in die interne MQTT-Sendewarteschlange.
 *
 * Nicht-blockierend, die Übertragung erfolgt asynchron durch den Client.
 *
 * @param topic   MQTT-Topic (nullterminiert).
 * @param data    Payload-Pointer (kann Binärdaten enthalten).
 * @param len     Länge der Payload in Bytes.
 * @param qos     Quality of Service (0, 1 oder 2; je nach Broker/Client-Unterstützung).
 * @param retain  Retain-Flag (0/1).
 */
void mqtt_enqueue(const char *topic, const char *data, int len, int qos, int retain);

#endif //MQTT_H