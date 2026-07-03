#include "imu_bmi270.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"
#include "solar_os_board.h"

/* Minimal BMI270 6-axis IMU driver: chip ID check, mandatory config-blob
 * upload (the feature-engine firmware; without it the chip never leaves
 * "initialization" state and accel/gyro registers stay at 0), then enable
 * accel+gyro at a fixed range/ODR and expose raw reads.
 * ponytail: no FIFO, no interrupts, no gesture/step features -- just the
 * "imu" shell command needs numbers. Upgrade to Bosch's full SensorAPI if
 * motion features are ever wanted. */

#define BMI270_ADDR SOLAR_OS_BOARD_IMU_ADDR

#define BMI270_REG_CHIP_ID 0x00
#define BMI270_REG_ACC_X_LSB 0x0C
#define BMI270_REG_GYR_X_LSB 0x12
#define BMI270_REG_INTERNAL_STATUS 0x21
#define BMI270_REG_ACC_CONF 0x40
#define BMI270_REG_GYR_CONF 0x42
#define BMI270_REG_INIT_CTRL 0x59
#define BMI270_REG_INIT_ADDR_0 0x5B
#define BMI270_REG_INIT_ADDR_1 0x5C
#define BMI270_REG_INIT_DATA 0x5E
#define BMI270_REG_PWR_CONF 0x7C
#define BMI270_REG_PWR_CTRL 0x7D
#define BMI270_REG_CMD 0x7E

#define BMI270_CHIP_ID 0x24
#define BMI270_CMD_SOFT_RESET 0xB6
#define BMI270_INTERNAL_STATUS_INIT_OK 0x01

/* ACC_CONF: odr=100Hz(0x08), bwp=normal_avg4(0x02<<4), perf=high(1<<7).
 * ACC_RANGE (0x41, adjacent register, written in the same 2-byte burst): 4g. */
#define BMI270_ACC_CONF_BYTES {0xA8, 0x01}
/* GYR_CONF: odr=100Hz(0x08), bwp=normal(0x02<<4), noise/filter perf high.
 * GYR_RANGE (0x43, adjacent): 1000 dps (range field is bits[2:0]). */
#define BMI270_GYR_CONF_BYTES {0xE8, 0x01}

#define ACCEL_4G_LSB_PER_G 8192.0f
#define GYRO_1000DPS_LSB_PER_DPS 32.768f

#include "bmi270_config.inc"

static const char *TAG = "imu_bmi270";
static bool imu_ready;

static esp_err_t bmi270_upload_config(void)
{
    /* PWR_CONF: disable advanced power save before configuring. */
    uint8_t pwr_conf = 0x00;
    ESP_RETURN_ON_ERROR(i2c_bus_write_reg(BMI270_ADDR, BMI270_REG_PWR_CONF, &pwr_conf, 1),
                        TAG, "pwr_conf failed");
    vTaskDelay(pdMS_TO_TICKS(1));

    uint8_t init_ctrl = 0x00;
    ESP_RETURN_ON_ERROR(i2c_bus_write_reg(BMI270_ADDR, BMI270_REG_INIT_CTRL, &init_ctrl, 1),
                        TAG, "init_ctrl(0) failed");

    /* Config blob loads in 256-byte bursts at consecutive 16-bit word
     * addresses (INIT_ADDR_0/1 hold the burst's start word offset). */
    const size_t burst = 256;
    for (size_t offset = 0; offset < sizeof(bmi270_config_file); offset += burst) {
        const size_t chunk = (sizeof(bmi270_config_file) - offset) < burst ?
            (sizeof(bmi270_config_file) - offset) : burst;
        const uint16_t word_addr = (uint16_t)(offset / 2);
        const uint8_t addr_bytes[2] = {
            (uint8_t)(word_addr & 0x0F),
            (uint8_t)(word_addr >> 4),
        };
        ESP_RETURN_ON_ERROR(
            i2c_bus_write_reg(BMI270_ADDR, BMI270_REG_INIT_ADDR_0, addr_bytes, sizeof(addr_bytes)),
            TAG, "init_addr failed");
        ESP_RETURN_ON_ERROR(
            i2c_bus_write_reg(BMI270_ADDR, BMI270_REG_INIT_DATA,
                              &bmi270_config_file[offset], chunk),
            TAG, "init_data burst failed");
    }

    init_ctrl = 0x01;
    ESP_RETURN_ON_ERROR(i2c_bus_write_reg(BMI270_ADDR, BMI270_REG_INIT_CTRL, &init_ctrl, 1),
                        TAG, "init_ctrl(1) failed");
    vTaskDelay(pdMS_TO_TICKS(20)); /* datasheet: config load takes ~20ms */

    uint8_t status = 0;
    ESP_RETURN_ON_ERROR(
        i2c_bus_read_reg(BMI270_ADDR, BMI270_REG_INTERNAL_STATUS, &status, 1), TAG,
        "internal_status read failed");
    if ((status & 0x0F) != BMI270_INTERNAL_STATUS_INIT_OK) {
        ESP_LOGE(TAG, "config load failed, internal_status=0x%02x", status);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t imu_bmi270_init(void)
{
    if (imu_ready) {
        return ESP_OK;
    }

    esp_err_t err = i2c_bus_init();
    if (err != ESP_OK) {
        return err;
    }

    uint8_t chip_id = 0;
    err = i2c_bus_read_reg(BMI270_ADDR, BMI270_REG_CHIP_ID, &chip_id, 1);
    if (err != ESP_OK) {
        return err;
    }
    if (chip_id != BMI270_CHIP_ID) {
        ESP_LOGW(TAG, "unexpected chip id 0x%02x (expected 0x%02x)", chip_id, BMI270_CHIP_ID);
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t reset_cmd = BMI270_CMD_SOFT_RESET;
    err = i2c_bus_write_reg(BMI270_ADDR, BMI270_REG_CMD, &reset_cmd, 1);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(2));

    err = bmi270_upload_config();
    if (err != ESP_OK) {
        return err;
    }

    /* Enable accelerometer + gyroscope (bits 2 and 1 of PWR_CTRL). */
    uint8_t pwr_ctrl = 0x06;
    err = i2c_bus_write_reg(BMI270_ADDR, BMI270_REG_PWR_CTRL, &pwr_ctrl, 1);
    if (err != ESP_OK) {
        return err;
    }

    const uint8_t acc_conf[2] = BMI270_ACC_CONF_BYTES;
    err = i2c_bus_write_reg(BMI270_ADDR, BMI270_REG_ACC_CONF, acc_conf, sizeof(acc_conf));
    if (err != ESP_OK) {
        return err;
    }
    const uint8_t gyr_conf[2] = BMI270_GYR_CONF_BYTES;
    err = i2c_bus_write_reg(BMI270_ADDR, BMI270_REG_GYR_CONF, gyr_conf, sizeof(gyr_conf));
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(50)); /* let the first samples settle */
    imu_ready = true;
    ESP_LOGI(TAG, "BMI270 ready");
    return ESP_OK;
}

esp_err_t imu_bmi270_read(imu_bmi270_sample_t *sample)
{
    if (sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!imu_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t accel_raw[6];
    ESP_RETURN_ON_ERROR(
        i2c_bus_read_reg(BMI270_ADDR, BMI270_REG_ACC_X_LSB, accel_raw, sizeof(accel_raw)), TAG,
        "accel read failed");
    uint8_t gyro_raw[6];
    ESP_RETURN_ON_ERROR(
        i2c_bus_read_reg(BMI270_ADDR, BMI270_REG_GYR_X_LSB, gyro_raw, sizeof(gyro_raw)), TAG,
        "gyro read failed");

    for (int axis = 0; axis < 3; axis++) {
        const int16_t accel_counts =
            (int16_t)((accel_raw[axis * 2 + 1] << 8) | accel_raw[axis * 2]);
        const int16_t gyro_counts =
            (int16_t)((gyro_raw[axis * 2 + 1] << 8) | gyro_raw[axis * 2]);
        sample->accel_g[axis] = (float)accel_counts / ACCEL_4G_LSB_PER_G;
        sample->gyro_dps[axis] = (float)gyro_counts / GYRO_1000DPS_LSB_PER_DPS;
    }

    return ESP_OK;
}
