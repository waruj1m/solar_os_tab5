#include "bmi270.h"

#include "esp_log.h"
#include "i2c_bus.h"

static const char *TAG = "bmi270";

#define BMI270_REG_CHIP_ID      0x00
#define BMI270_REG_ERR_REG      0x02
#define BMI270_REG_STATUS       0x03
#define BMI270_REG_DATA_START   0x0C
#define BMI270_REG_SENSORTIME   0x18
#define BMI270_REG_CMD          0x7E
#define BMI270_CHIP_ID_VALUE    0x24

static bool initialized = false;

esp_err_t bmi270_init(void)
{
    uint8_t chip_id = 0;
    esp_err_t ret = i2c_bus_read_reg(BMI270_I2C_ADDRESS, BMI270_REG_CHIP_ID, &chip_id, 1);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "BMI270 not found at 0x%02x", BMI270_I2C_ADDRESS);
        return ret;
    }

    if (chip_id != BMI270_CHIP_ID_VALUE) {
        ESP_LOGW(TAG, "BMI270 unexpected chip ID: 0x%02x", chip_id);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "BMI270 found, chip ID: 0x%02x", chip_id);
    initialized = true;
    return ESP_OK;
}

esp_err_t bmi270_read(bmi270_data_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    data->data_ready = false;
    data->temperature_c = 25.0f;
    return ESP_OK;
}
