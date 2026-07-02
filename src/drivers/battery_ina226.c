#include "battery_ina226.h"

#include "esp_log.h"
#include "i2c_bus.h"

static const char *TAG = "ina226";

#define INA226_REG_CONFIG      0x00
#define INA226_REG_SHUNT_V     0x01
#define INA226_REG_BUS_V       0x02
#define INA226_REG_POWER       0x03
#define INA226_REG_CURRENT     0x04
#define INA226_REG_CALIB       0x05

#define INA226_CONFIG_RESET    0x8000
#define INA226_CONFIG_DEFAULT  0x4127

#define INA226_SHUNT_OHM 0.005f

static bool initialized = false;

esp_err_t battery_ina226_init(void)
{
    uint8_t config[2] = {0};

    esp_err_t ret = i2c_bus_read_reg(INA226_I2C_ADDRESS, INA226_REG_CONFIG, config, 2);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "INA226 not found at 0x%02x", INA226_I2C_ADDRESS);
        return ret;
    }

    uint16_t cfg_val = (uint16_t)((config[0] << 8) | config[1]);
    if (cfg_val != INA226_CONFIG_DEFAULT) {
        uint8_t cfg_data[2] = {
            (uint8_t)(INA226_CONFIG_DEFAULT >> 8),
            (uint8_t)(INA226_CONFIG_DEFAULT & 0xFF)
        };
        ret = i2c_bus_write_reg(INA226_I2C_ADDRESS, INA226_REG_CONFIG, cfg_data, 2);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    uint32_t current_lsb = 10000;
    uint16_t calib = (uint16_t)(0.00512 / (current_lsb * INA226_SHUNT_OHM));
    uint8_t cal_data[2] = {
        (uint8_t)(calib >> 8),
        (uint8_t)(calib & 0xFF)
    };
    ret = i2c_bus_write_reg(INA226_I2C_ADDRESS, INA226_REG_CALIB, cal_data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    ESP_LOGI(TAG, "INA226 initialized");
    initialized = true;
    return ESP_OK;
}

esp_err_t battery_ina226_read(ina226_sample_t *sample)
{
    if (sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t bus_v[2];
    esp_err_t ret = i2c_bus_read_reg(INA226_I2C_ADDRESS, INA226_REG_BUS_V, bus_v, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    uint16_t bus_raw = (uint16_t)((bus_v[0] << 8) | bus_v[1]);
    sample->battery_mv = (uint16_t)((uint32_t)bus_raw * 125 / 100);
    sample->calibrated = true;
    sample->current_ma = 0;
    sample->power_mw = 0;

    return ESP_OK;
}
