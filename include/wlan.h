#ifndef WLAN_H
#define WLAN_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"

/**
 * Zentraler Event-Handler für Wi-Fi/IP-Ereignisse im STA-Modus.
 *
 * Reagiert u. a. auf:
 * - WIFI_EVENT_STA_START: startet Verbindungsaufbau (esp_wifi_connect)
 * - WIFI_EVENT_STA_DISCONNECTED: setzt Reconnect an und löscht Verbindungs-Flag
 * - IP_EVENT_STA_GOT_IP: setzt Verbindungs-Flag nach erfolgreicher IP-Zuweisung
 *
 * @param arg        Benutzerargument (nicht verwendet).
 * @param event_base Ereignisbasis (WIFI_EVENT oder IP_EVENT).
 * @param event_id   Ereigniskennung.
 * @param event_data Zeiger auf event-spezifische Datenstruktur.
 */
void eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

// ... existing code ...

/**
 * Registriert die benötigten Event-Handler für STA-Betrieb am Default Event Loop.
 *
 * Muss nach esp_event_loop_create_default() aufgerufen werden.
 * Registriert:
 * - WIFI_EVENT, ESP_EVENT_ANY_ID -> eventHandler
 * - IP_EVENT, IP_EVENT_STA_GOT_IP -> eventHandler
 */
void initStaEventHandlers(void);

/**
 * Konfiguriert den Station-Mode für WPA2-Enterprise (802.1X/EAP).
 *
 * Setzt SSID und EAP-Zugangsdaten und aktiviert den Enterprise-Client.
 * Erfordert passende KConfig-Definitionen (z. B. CONFIG_WPA2E_*).
 */
void initWPA2Enterprise(void);

/**
 * Konfiguriert den Station-Mode für WPA/WPA2-Personal (PSK).
 *
 * Setzt SSID/Passwort sowie sinnvolle Default-Sicherheitsparameter.
 */
void initWPA2Personal(void);

/**
 * Initialisiert den WLAN-STA-Stack und startet den Verbindungsaufbau.
 *
 * Ablauf:
 * - esp_netif und Default-Event-Loop initialisieren
 * - Default STA-Interface anlegen
 * - Wi-Fi initialisieren und auf STA setzen
 * - Event-Handler registrieren
 * - WPA2 (Enterprise/Personal) konfigurieren
 * - Wi-Fi starten
 */
void initSTA(void);

/**
 * Blockiert bis der STA eine IP erhalten hat oder das Timeout erreicht ist.
 *
 * Voraussetzung: initSTA() wurde vorher aufgerufen.
 *
 * @param timeout Maximale Wartezeit in OS-Ticks (portMAX_DELAY für unbegrenzt).
 * @return true, wenn innerhalb des Timeouts verbunden (IP erhalten); sonst false.
 */
bool waitForSTAConnected(TickType_t timeout);

#endif // WLAN_H
