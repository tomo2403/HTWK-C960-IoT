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

extern i2c_port_t i2c_num;
extern sgp30_dev_t main_sgp30_sensor;
extern bmx280_t* bmx280;

esp_err_t i2c_master_driver_initialize(void);

/**
 * @brief generic function for reading I2C data
 *
 * @param reg_addr register an address to read from
 * @param reg_data pointer to save the data read
 * @param len length of data to be read
 * @param intf_ptr
 *
 * >init: dev->intf_ptr = &dev_addr;
 *
 * @return ESP_OK if reading was successful
 */
int8_t main_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);

/**
 * @brief generic function for writing data via I2C
 *
 * @param reg_addr register an address to write to
 * @param reg_data register data to be written
 * @param len length of data to be written
 * @param intf_ptr
 *
 * @return ESP_OK if writing was successful
 */
int8_t main_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);

void sensor_sgp30_init();

void sensor_bmx280_init();

#endif // SENSORS_H
