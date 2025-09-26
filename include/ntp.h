#ifndef HTWK_C960_IOT_NTP_H
#define HTWK_C960_IOT_NTP_H

#include "esp_sntp.h"
#include <esp_log.h>

/**
 * POSIX-Zeitzonen-Definition für Mitteleuropa.
 *
 * Format:
 * - CET-1: Basisoffset (UTC+1)
 * - CEST: Sommerzeit-Kennung
 * - M3.5.0: Beginn Sommerzeit (März, letzte Woche, Sonntag)
 * - M10.5.0/3: Ende Sommerzeit (Oktober, letzte Woche, Sonntag, 03:00)
 */
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

/**
 * Callback nach erfolgreicher SNTP-Zeitsynchronisierung.
 *
 * Kann genutzt werden, um Log-Ausgaben zu erzeugen oder zeitabhängige
 * Komponenten neu zu initialisieren.
 *
 * Thread-Kontext: Wird aus dem SNTP/Netzwerk-Kontext aufgerufen.
 *
 * @param tv Zeiger auf die neue Systemzeit.
 */
void time_sync_notification_cb(struct timeval *tv);

/**
 * Initialisiert SNTP und stößt die NTP-Zeitsynchronisation an.
 *
 * Ablauf:
 * - Setzt Betriebsmodus und Default-Server ("pool.ntp.org").
 * - Registriert den Synchronisations-Callback.
 * - Initialisiert SNTP-Client.
 * - Setzt Zeitzone gemäß TIMEZONE und aktualisiert libc-Zeit via tzset().
 * - Wartet kurz wiederholt auf gültige Systemzeit und protokolliert Ergebnis.
 */
void ntp_obtain_time();

#endif //HTWK_C960_IOT_NTP_H
