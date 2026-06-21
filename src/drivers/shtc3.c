#include "shtc3.h"

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"

#define SHTC3_CMD_READ_ID 0xefc8
#define SHTC3_CMD_SOFT_RESET 0x805d
#define SHTC3_CMD_SLEEP 0xb098
#define SHTC3_CMD_WAKEUP 0x3517
#define SHTC3_CMD_MEASURE_T_RH_POLLING 0x7866
#define SHTC3_TEMPERATURE_OFFSET_C (-4.0f)

static uint8_t shtc3_crc(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xff;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if ((crc & 0x80) != 0) {
                crc = (uint8_t)((crc << 1) ^ 0x31);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static bool shtc3_crc_ok(const uint8_t *data, uint8_t checksum)
{
    return shtc3_crc(data, 2) == checksum;
}

static esp_err_t shtc3_write_cmd(uint16_t command)
{
    const uint8_t data[2] = {
        (uint8_t)(command >> 8),
        (uint8_t)(command & 0xff),
    };

    return i2c_bus_transmit(SHTC3_ADDRESS, data, sizeof(data));
}

static esp_err_t shtc3_wakeup(void)
{
    const esp_err_t ret = shtc3_write_cmd(SHTC3_CMD_WAKEUP);
    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    return ret;
}

static esp_err_t shtc3_sleep(void)
{
    return shtc3_write_cmd(SHTC3_CMD_SLEEP);
}

esp_err_t shtc3_read_id(uint16_t *id)
{
    if (id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = shtc3_wakeup();
    if (ret != ESP_OK) {
        return ret;
    }

    const uint8_t command[2] = {
        (uint8_t)(SHTC3_CMD_READ_ID >> 8),
        (uint8_t)(SHTC3_CMD_READ_ID & 0xff),
    };
    uint8_t data[3] = {0};

    ret = i2c_bus_transmit_receive(SHTC3_ADDRESS, command, sizeof(command), data, sizeof(data));
    esp_err_t sleep_ret = shtc3_sleep();
    if (ret != ESP_OK) {
        return ret;
    }
    if (sleep_ret != ESP_OK) {
        return sleep_ret;
    }
    if (!shtc3_crc_ok(data, data[2])) {
        return ESP_ERR_INVALID_CRC;
    }

    *id = (uint16_t)((data[0] << 8) | data[1]);
    return ESP_OK;
}

esp_err_t shtc3_init(void)
{
    esp_err_t ret = shtc3_wakeup();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = shtc3_write_cmd(SHTC3_CMD_SOFT_RESET);
    if (ret != ESP_OK) {
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    uint16_t id = 0;
    ret = shtc3_read_id(&id);
    if (ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

esp_err_t shtc3_read_measurement(shtc3_measurement_t *measurement)
{
    if (measurement == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = shtc3_wakeup();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = shtc3_write_cmd(SHTC3_CMD_MEASURE_T_RH_POLLING);
    if (ret != ESP_OK) {
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t data[6] = {0};
    ret = i2c_bus_receive(SHTC3_ADDRESS, data, sizeof(data));
    esp_err_t sleep_ret = shtc3_sleep();
    if (ret != ESP_OK) {
        return ret;
    }
    if (sleep_ret != ESP_OK) {
        return sleep_ret;
    }
    if (!shtc3_crc_ok(&data[0], data[2]) || !shtc3_crc_ok(&data[3], data[5])) {
        return ESP_ERR_INVALID_CRC;
    }

    const uint16_t raw_temp = (uint16_t)((data[0] << 8) | data[1]);
    const uint16_t raw_humidity = (uint16_t)((data[3] << 8) | data[4]);

    measurement->temperature_c = ((175.0f * (float)raw_temp) / 65536.0f) - 45.0f + SHTC3_TEMPERATURE_OFFSET_C;
    measurement->humidity_percent = (100.0f * (float)raw_humidity) / 65536.0f;

    uint16_t id = 0;
    if (shtc3_read_id(&id) == ESP_OK) {
        measurement->id = id;
    } else {
        measurement->id = 0;
    }

    return ESP_OK;
}
