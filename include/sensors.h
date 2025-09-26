#ifndef SENSORS_H
#define SENSORS_H

#include "driver/i2c.h"
#include "bmx280.h"
#include "SGP30.h"

#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

/**
 * Global ausgewählter I2C-Port für den Master.
 * Wird in i2c_master_driver_initialize() konfiguriert.
 */
extern i2c_port_t i2c_num;

/**
 * Globale Instanz des SGP30-Sensor-Handles.
 * Nach sensor_sgp30_init() initialisiert und einsatzbereit.
 */
extern sgp30_dev_t main_sgp30_sensor;

/**
 * Globale Handle-Pointer-Referenz für BMX280 (BME280/BMP280).
 * Nach sensor_bmx280_init() erstellt/konfiguriert.
 */
extern bmx280_t* bmx280;

/**
 * Initialisiert den I2C-Master-Treiber.
 *
 * Konfiguriert SDA/SCL, Pull-Ups und Busfrequenz und installiert den
 * Treiber für den in i2c_num gewählten Port.
 *
 * @return ESP_OK bei Erfolg, sonst Fehlercode von i2c_driver_install().
 */
esp_err_t i2c_master_driver_initialize(void);

/**
 * Generische I2C-Lesefunktion (Callback-kompatibel für Sensor-Treiber).
 *
 * Liest len Bytes ab der Registeradresse reg_addr vom Slave unter der
 * Adresse, die via intf_ptr übergeben wurde (z. B. dev->intf_ptr).
 *
 * Besonderheit: Wenn reg_addr == 0xFF bzw. 0xff, wird kein Register-Write
 * vorangestellt (direktes Lesen vom Gerät).
 *
 * @param reg_addr  Registeradresse (oder 0xFF für kein Register).
 * @param reg_data  Ausgabepuffer.
 * @param len       Anzahl der zu lesenden Bytes.
 * @param intf_ptr  Zeiger auf die 7-bit Slave-Adresse (uint8_t*).
 * @return 0/ESP_OK bei Erfolg, sonst Fehlercode von i2c_master_cmd_begin().
 */
int8_t main_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);

/**
 * Generische I2C-Schreibfunktion (Callback-kompatibel für Sensor-Treiber).
 *
 * Schreibt len Bytes an die Registeradresse reg_addr des Slaves unter der
 * Adresse, die via intf_ptr übergeben wurde (z. B. dev->intf_ptr).
 *
 * Besonderheit: Wenn reg_addr == 0xFF, wird kein Register-Byte gesendet
 * (direkter Payload-Write).
 *
 * @param reg_addr  Registeradresse (oder 0xFF für kein Register).
 * @param reg_data  Eingabepuffer mit zu schreibenden Daten.
 * @param len       Anzahl der zu schreibenden Bytes.
 * @param intf_ptr  Zeiger auf die 7-bit Slave-Adresse (uint8_t*).
 * @return 0/ESP_OK bei Erfolg, sonst Fehlercode von i2c_master_cmd_begin().
 */
int8_t main_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);

/**
 * Initialisiert den SGP30-Luftqualitätssensor.
 *
 * - Bindet die generischen I2C-Callbacks ein.
 * - Führt eine Anlauf-Kalibrierung (IAQ) durch und protokolliert Baselines.
 * Nach Abschluss sind eCO2/TVOC-Messungen über main_sgp30_sensor verfügbar.
 */
void sensor_sgp30_init();

/**
 * Erstellt und konfiguriert den BMX280-Treiber (BME280/BMP280).
 *
 * - Erzeugt die Treiberinstanz auf dem I2C-Bus.
 * - Lädt Kalibrierung, setzt Standardkonfiguration (IIR, Standby).
 * - Versetzt den Sensor abschließend in den Schlafmodus (BMX280_MODE_SLEEP).
 * Nach Bedarf kann der Modus später auf FORCED/NORMAL geändert werden.
 */
void sensor_bmx280_init();

#endif // SENSORS_H