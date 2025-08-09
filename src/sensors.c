#include "sensors.h"

i2c_port_t i2c_num = CONFIG_I2C_MASTER_PORT_NUM;
sgp30_dev_t main_sgp30_sensor;
bmx280_t* bmx280;

static const char *TAG = "SensorManager";

esp_err_t i2c_master_driver_initialize(void)
{
    const int i2c_master_port = i2c_num;
    i2c_config_t conf;

    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = CONFIG_I2C_MASTER_SDA;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = CONFIG_I2C_MASTER_SCL;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = CONFIG_I2C_MASTER_FREQUENCY;
    conf.clk_flags = 0;

    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
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
    sgp30_init(&main_sgp30_sensor, main_i2c_read, main_i2c_write);

    ESP_LOGI(TAG, "SGP30 Calibrating...");
    for (int i = 0; i < 20; i++)
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

void sensor_bmx280_init()
{
    ESP_LOGI(TAG, "BMX280 main task initializing...");
    bmx280 = bmx280_create(I2C_NUM_0);

    if (!bmx280) {
        ESP_LOGE("test", "Could not create bmx280 driver.");
        return;
    }

    ESP_ERROR_CHECK(bmx280_init(bmx280));

    bmx280_config_t bmx_cfg = BMX280_DEFAULT_CONFIG;
    ESP_ERROR_CHECK(bmx280_configure(bmx280, &bmx_cfg));
    ESP_ERROR_CHECK(bmx280_setMode(bmx280, BMX280_MODE_CYCLE));
}
