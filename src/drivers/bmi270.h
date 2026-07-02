#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define BMI270_I2C_ADDRESS 0x68

typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float temperature_c;
    bool data_ready;
} bmi270_data_t;

esp_err_t bmi270_init(void);
esp_err_t bmi270_read(bmi270_data_t *data);
