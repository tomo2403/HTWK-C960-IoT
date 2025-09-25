# HTWK-C960-IoT

## Kurzfassung
Die Umweltkarre ist ein ESP32-basiertes IoT-Fahrzeug, das Umwelt- und Luftqualitätsdaten (TVOC, eCO2, Temperatur, Druck, optional Feuchte) erfasst und sie per MQTT veröffentlicht. Ein zweiter ESP32 steuert das Fahrzeug latenzarm über ESPNOW. Beim Start initialisiert das System WLAN und NTP, entdeckt Peers per Broadcast-Handshake, liest Sensoren zyklisch aus und puffert MQTT-Publishes für eine robuste Übertragung. Im Backend (Docker Compose) werden die Daten via Telegraf in InfluxDB gespeichert und in Grafana visualisiert. Tests verifizieren u. a. die Zeitsynchronisation; in der Praxis zeigte sich eine stabile Mess- und Steuerfunktion. Bekannte Einschränkungen betreffen u. a. die optional deaktivierte Feuchtemessung und 2.4-GHz-Interferenzen. Geplante Schritte: Motoranbindung mit Sicherheitslogik, persistente Joystick-Kalibrierung, TLS-gesichertes MQTT und OTA-Updates.

Vollständiger Bericht: docs/abschlussbericht.md

---

## Überblick
- Zweck: Demonstrator für IoT-Sensorik, MQTT-Telemetrie und latenzarme Funksteuerung (ESPNOW).
- Architektur: ESP32 „Car“ (Sensorik + MQTT) und ESP32 „Controller“ (Joystick + ESPNOW). NTP für Zeitbasis, Docker-Backend für Persistenz/Visualisierung.
- Bild/Diagramm: siehe docs/architecture.png

## Hardware
- Boards: ESP32-S3 oder ESP32-C6 Devkit (getestet). Optionales Foto: ESP32-C6-DEV-KIT-N805.jpg
- Sensoren: BMX/BMP280 (Temperatur, Druck, optional Feuchte), SGP30 (TVOC, eCO2)
- Controller: Joystick + Taster am zweiten ESP32 (ESPNOW)
- Optional: Motor/H-Brücke (geplant)

## Software & Build
- Tooling: PlatformIO (ESP-IDF), FreeRTOS, C
- Targets: in platformio.ini auswählen (passendes Board/Env)
- Build & Flash: über PlatformIO ausführen; serielle Konsole für Logs öffnen

## Konfiguration
- WLAN/MQTT: include/secrets.h auf Basis von include/example.secrets.h ausfüllen
- Topics: Standardmäßig unter /sensor/{tvoc,eco2,temperature,pressure,humidity}
- Intervalle: Sensorpublish ca. alle 10 s (siehe src/sensors.c / src/main.c)
- Zeit: ntp_obtain_time() beim Start (siehe src/ntp.c)

## Backend (optional, Docker Compose)
- Services: mosquitto (1883), telegraf, influxdb (8086), grafana (3000)
- Start: in backend/ wechseln und docker compose up -d
- Grafana: InfluxDB v2 Data Source einrichten (siehe docs/abschlussbericht.md, Abschnitt Repository-Überblick)

## Steuerung (ESPNOW)
- Discovery: Broadcast-Handshake mit Token, danach Unicast-ACK
- Rollen: Controller sendet Joystick-Frames; Car empfängt und setzt Befehle um
- Latenz: typischerweise < 100 ms in 2.4 GHz-Umgebung

## MQTT-Topics
- /sensor/tvoc
- /sensor/eco2
- /sensor/temperature
- /sensor/pressure
- /sensor/humidity (optional, je nach Build-Flag)

## Troubleshooting
- MQTT kommt nicht an: Broker-Host/Port prüfen; mosquitto_sub -t "/sensor/#" -v testen
- Telegraf → Influx: telegraf.conf und Container-Logs (docker logs telegraf) prüfen
- Ports belegt: prüfen, ob lokale Dienste laufen (Mosquitto/Influx/Grafana)

## Sicherheit
- MQTT: allow_anonymous=true nur für lokale Demos geeignet; für Produktion Benutzer/TLS aktivieren
- Secrets: *.env und Tokens ändern/rotieren; Volumes sichern und Updates einspielen

## Lizenz
- Lizenz siehe LICENSE; Drittlizenzen in components/*

## Debugging
### Clone dependencies
```bash
  git clone https://github.com/utkumaden/esp-idf-bmx280.git components/bmx280
  git clone https://github.com/tomo2403/esp32_SGP30.git components/sgp30
```