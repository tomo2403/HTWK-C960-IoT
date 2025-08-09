#include "sensors.h"

i2c_port_t i2c_num = I2C_MASTER_NUM;
sgp30_dev_t main_sgp30_sensor;

static const char *TAG = "SensorManager";

esp_err_t i2c_master_driver_initialize(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;

    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = 0;

    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

int8_t main_i2c_read(const uint8_t reg_addr, uint8_t *reg_data, const uint32_t len, void *intf_ptr)
{
    // *intf_ptr = dev->intf_ptr
    int8_t ret = 0; /* Return 0 for Success, non-zero for failure */

    if (len == 0)
    {
        return ESP_OK;
    }

    const uint8_t chip_addr = *(uint8_t *) intf_ptr;

    const i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);

    if (reg_addr != 0xff)
    {
        i2c_master_write_byte(cmd, (chip_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
        i2c_master_start(cmd);
    }

    i2c_master_write_byte(cmd, (chip_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);

    if (len > 1)
    {
        i2c_master_read(cmd, reg_data, len - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, reg_data + len - 1, NACK_VAL);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);

    i2c_cmd_link_delete(cmd);

    return ret;
}

int8_t main_i2c_write(const uint8_t reg_addr, const uint8_t *reg_data, const uint32_t len, void *intf_ptr)
{
    int8_t ret = 0; /* Return 0 for Success, non-zero for failure */

    const uint8_t chip_addr = *(uint8_t *) intf_ptr;

    const i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (chip_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);

    if (reg_addr != 0xFF)
    {
        i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
    }

    i2c_master_write(cmd, reg_data, len, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);

    i2c_cmd_link_delete(cmd);

    return ret;
}

void sensor_sgp30_init()
{
    ESP_LOGI(TAG, "SGP30 main task initializing...");
    i2c_master_driver_initialize();
    sgp30_init(&main_sgp30_sensor, main_i2c_read, main_i2c_write);

    ESP_LOGI(TAG, "SGP30 Calibrating...");
    for (int i = 0; i < 30; i++)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        sgp30_IAQ_measure(&main_sgp30_sensor);
        ESP_LOGD(TAG, "SGP30 Calibrating... TVOC: %d,  eCO2: %d", main_sgp30_sensor.TVOC, main_sgp30_sensor.eCO2);
    }

    // Read initial baselines
    uint16_t eco2_baseline, tvoc_baseline;
    sgp30_get_IAQ_baseline(&main_sgp30_sensor, &eco2_baseline, &tvoc_baseline);
    ESP_LOGI(TAG, "BASELINES - TVOC: %d,  eCO2: %d", tvoc_baseline, eco2_baseline);
}
