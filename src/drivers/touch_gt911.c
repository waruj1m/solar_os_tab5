#include "touch_gt911.h"

#include "esp_log.h"
#include "i2c_bus.h"

static const char *TAG = "gt911";

#define GT911_REG_COMMAND    0x8040
#define GT911_REG_CONFIG     0x8047
#define GT911_REG_DATA       0x814E

#define GT911_READY          0x00
#define GT911_READ_POINTS    0x82
#define GT911_CLEAR_TOUCH    0x00

static uint8_t gt911_addr = GT911_I2C_ADDR_1;
static bool initialized = false;

static bool probe_address(uint8_t addr)
{
    return i2c_bus_probe(addr) == ESP_OK;
}

esp_err_t touch_gt911_init(void)
{
    if (!probe_address(GT911_I2C_ADDR_1) && !probe_address(GT911_I2C_ADDR_2)) {
        ESP_LOGW(TAG, "GT911 not found at 0x%02x or 0x%02x",
                 GT911_I2C_ADDR_1, GT911_I2C_ADDR_2);
        return ESP_ERR_NOT_FOUND;
    }

    if (probe_address(GT911_I2C_ADDR_1)) {
        gt911_addr = GT911_I2C_ADDR_1;
    } else {
        gt911_addr = GT911_I2C_ADDR_2;
    }

    ESP_LOGI(TAG, "GT911 found at 0x%02x", gt911_addr);

    uint8_t cmd = GT911_READY;
    i2c_bus_write_reg(gt911_addr, GT911_REG_COMMAND >> 8, &cmd, 1);
    initialized = true;

    return ESP_OK;
}

esp_err_t touch_gt911_read(gt911_touch_data_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t status = 0;
    uint16_t reg = GT911_REG_DATA;
    uint8_t reg_raw[2] = {
        (uint8_t)(reg >> 8),
        (uint8_t)(reg & 0xFF)
    };

    esp_err_t ret = i2c_bus_transmit_receive(gt911_addr, reg_raw, 2, &status, 1);
    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t point_count = (uint8_t)(status & 0x0F);
    data->point_count = point_count;

    uint8_t buffer[15];
    ret = i2c_bus_transmit_receive(gt911_addr, reg_raw, 2, buffer, 15);
    if (ret != ESP_OK) {
        return ret;
    }

    data->gesture = buffer[0];

    for (int i = 0; i < 5 && i < point_count; i++) {
        int off = 1 + i * 3;
        if (off + 2 < 15) {
            data->points[i].touched = true;
            data->points[i].x = (uint16_t)(buffer[off + 1] << 8 | buffer[off]);
            data->points[i].y = (uint16_t)(buffer[off + 1 + 1] << 8 | buffer[off + 1]);
            data->points[i].size = 0;
        }
    }

    uint8_t clear = GT911_CLEAR_TOUCH;
    i2c_bus_write_reg(gt911_addr, GT911_REG_COMMAND >> 8, &clear, 1);

    return ESP_OK;
}
