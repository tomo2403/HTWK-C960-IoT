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

// Empfangs-Callback: wird mit vollständigen, reassemblierten Nutzdaten aufgerufen
// mac: Quell-MAC (6 Bytes)
// data/len: vollständige Nutzlast
// user_ctx: Kontext, der bei espnow_init übergeben wurde
typedef void (*espnow_recv_cb_t)(const uint8_t mac[6], const uint8_t* data, size_t len, void* user_ctx);

// Initialisiert ESPNOW auf einem bestehenden Wi-Fi-Interface (z. B. WIFI_IF_STA oder WIFI_IF_AP).
// Diese Funktion verändert die bestehende Wi-Fi-Verbindung nicht.
// - ifx: Interface, an das ESPNOW gebunden wird (WIFI_IF_STA empfohlen)
// - recv_cb: Callback für vollständige, reassemblierten Nachrichten
// - user_ctx: Benutzerkontext, an recv_cb durchgereicht
esp_err_t espnow_init(wifi_interface_t ifx, espnow_recv_cb_t recv_cb, void* user_ctx);

// Deinitialisiert ESPNOW und gibt Ressourcen frei
esp_err_t espnow_deinit(void);

// Fügt einen ESPNOW-Peer hinzu.
// - peer_mac: MAC-Adresse des Gegenübers (6 Bytes)
// - lmk: Optionaler 16-Byte-Schlüssel für Verschlüsselung (NULL, wenn keine Verschlüsselung)
// - encrypt: true, wenn Peer-Frames verschlüsselt werden sollen
esp_err_t espnow_add_peer(const uint8_t peer_mac[6], const uint8_t* lmk, bool encrypt);

// Entfernt einen ESPNOW-Peer
esp_err_t espnow_remove_peer(const uint8_t peer_mac[6]);

// Sendet beliebige Datenlänge an einen Peer. Automatische Fragmentierung.
// - peer_mac: Ziel-MAC (6 Bytes)
// - data/len: Nutzlast
esp_err_t espnow_send(const uint8_t peer_mac[6], const void* data, size_t len);

// Liefert die MAC des gebundenen Interfaces (z. B. STA-MAC) zurück.
// mac_out muss 6 Bytes groß sein.
esp_err_t espnow_get_our_mac(uint8_t mac_out[6]);

// Optional: Setzt eine gemeinsame PMK (Primary Master Key) für ESPNOW (wirkt global).
// key muss 16 Byte lang sein.
esp_err_t espnow_set_pmk(const uint8_t key[ESPNOW_KEY_LEN]);

#endif //HTWK_C960_IOT_ESPNOW_H