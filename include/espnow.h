#ifndef HTWK_C960_IOT_ESPNOW_H
#define HTWK_C960_IOT_ESPNOW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_wifi.h"


// Maximale Größe einer reassemblierten Nachricht (konfigurierbar)
#ifndef ESPNOW_MAX_MESSAGE_SIZE
#define ESPNOW_MAX_MESSAGE_SIZE 8192
#endif

// Maximale Anzahl gleichzeitiger Reassemblierungen pro Absender
#ifndef ESPNOW_MAX_INFLIGHT_MESSAGES
#define ESPNOW_MAX_INFLIGHT_MESSAGES 4
#endif

// Maximale Anzahl bekannter Peers, die wir verwalten
#ifndef ESPNOW_MAX_PEERS
#define ESPNOW_MAX_PEERS 1
#endif

// Länge eines Local Master Key (LMK) für verschlüsseltes ESPNOW
#define ESPNOW_KEY_LEN 16

/**
 * Empfangs-Callback für vollständig reassemblierten ESPNOW-Nutzdaten.
 *
 * Wird aufgerufen, sobald alle Fragmente einer Nachricht eingetroffen und korrekt
 * zusammengesetzt wurden.
 *
 * Thread-Kontext: Wird aus dem ESPNOW-Callback-Kontext aufgerufen.
 * Halte die Verarbeitung kurz oder delegiere in eine Task/Warteschlange.
 *
 * @param mac      Quell-MAC-Adresse (6 Bytes).
 * @param data     Zeiger auf vollständige Nutzdaten.
 * @param len      Länge der Nutzdaten in Byte.
 * @param user_ctx Der bei espnow_init übergebene Benutzerkontext.
 */
typedef void (*espnow_recv_cb_t)(const uint8_t mac[6], const uint8_t *data, size_t len, void *user_ctx);

/**
 * Initialisiert die ESPNOW-Schicht auf einem bestehenden Wi-Fi-Interface.
 *
 * Voraussetzungen:
 * - Wi-Fi muss bereits initialisiert und gestartet sein (z. B. STA/AP).
 * - Der Aufruf verändert die aktuelle Wi-Fi-Verbindung nicht.
 *
 * @param ifx      Wi-Fi-Interface (z. B. WIFI_IF_STA).
 * @param recv_cb  Callback für vollständig reassemblierten Empfang (kann NULL sein).
 * @param user_ctx Beliebiger Kontext, der im Callback unverändert durchgereicht wird.
 * @return ESP_OK bei Erfolg,
 *         ESP_ERR_INVALID_STATE wenn Wi-Fi nicht bereit,
 *         ESP_FAIL/esp_now Fehlercodes bei Initialisierungsproblemen.
 */
esp_err_t espnow_init(wifi_interface_t ifx, espnow_recv_cb_t recv_cb, void *user_ctx);

/**
 * Deinitialisiert ESPNOW und gibt alle intern allokierten Ressourcen frei.
 *
 * @return ESP_OK (idempotent).
 */
esp_err_t espnow_deinit(void);

/**
 * Fügt einen ESPNOW-Peer hinzu oder ersetzt vorhandene Konfiguration.
 *
 * Hinweis:
 * - Falls der Peer bereits existiert, wird er intern zuerst entfernt und dann neu angelegt.
 * - Verschlüsselung erfordert einen 16-Byte LMK-Schlüssel.
 *
 * @param peer_mac MAC-Adresse des Gegenübers (6 Bytes, darf nicht NULL sein).
 * @param lmk      Optionaler LMK-Schlüssel (16 Bytes) bei encrypt=true, sonst NULL.
 * @param encrypt  true, um Frames an diesen Peer zu verschlüsseln; false sonst.
 * @return ESP_OK bei Erfolg,
 *         ESP_ERR_INVALID_STATE wenn nicht initialisiert,
 *         ESP_ERR_INVALID_ARG bei ungültigen Parametern,
 *         sonst esp_now-spezifische Fehler.
 */
esp_err_t espnow_add_peer(const uint8_t peer_mac[6], const uint8_t *lmk, bool encrypt);

/**
 * Entfernt einen ESPNOW-Peer.
 *
 * @param peer_mac MAC-Adresse des zu entfernenden Peers (6 Bytes).
 * @return ESP_OK bei Erfolg,
 *         ESP_ERR_INVALID_STATE wenn nicht initialisiert,
 *         sonst esp_now-spezifische Fehler.
 */
esp_err_t espnow_remove_peer(const uint8_t peer_mac[6]);

/**
 * Sendet eine Nachricht beliebiger Länge an einen Peer.
 *
 * Implementierungshinweis:
 * - Daten werden automatisch in Fragmente aufgeteilt und am Empfänger reassembliert.
 * - Begrenzung durch ESPNOW_MAX_FRAGMENTS und ESPNOW_FRAGMENT_PAYLOAD; effektiv
 *   darf len nicht größer sein als ESPNOW_MAX_MESSAGE_SIZE.
 *
 * @param peer_mac Ziel-MAC (6 Bytes).
 * @param data     Zeiger auf Nutzdaten (kann bei len=0 NULL sein).
 * @param len      Länge der Nutzdaten in Byte.
 * @return ESP_OK bei Erfolg,
 *         ESP_ERR_INVALID_STATE wenn nicht initialisiert oder Parameter ungültig,
 *         ESP_ERR_NO_MEM wenn Nachricht zu groß,
 *         sonst esp_now-spezifische Fehler.
 */
esp_err_t espnow_send(const uint8_t peer_mac[6], const void *data, size_t len);

/**
 * Liefert die MAC-Adresse des gebundenen Interfaces (z. B. STA-MAC).
 *
 * @param mac_out Ausgabepuffer (6 Bytes).
 * @return ESP_OK bei Erfolg,
 *         ESP_ERR_INVALID_ARG wenn mac_out NULL ist,
 *         sonst Fehler von esp_wifi_get_mac().
 */
esp_err_t espnow_get_our_mac(uint8_t mac_out[6]);

/**
 * Setzt die globale PMK (Primary Master Key) für ESPNOW.
 *
 * Hinweis: Wirkt global für ESPNOW; Peer-spezifische Verschlüsselung nutzt zusätzlich LMK.
 *
 * @param key 16-Byte-PMK-Schlüssel.
 * @return ESP_OK bei Erfolg,
 *         ESP_ERR_INVALID_STATE wenn ESPNOW nicht initialisiert,
 *         sonst Fehler von esp_now_set_pmk().
 */
esp_err_t espnow_set_pmk(const uint8_t key[ESPNOW_KEY_LEN]);

#endif //HTWK_C960_IOT_ESPNOW_H
