#pragma once

#include <stdint.h>

#include "esp_err.h"

typedef struct {
    float accel_g[3];   /* X/Y/Z acceleration in g */
    float gyro_dps[3];  /* X/Y/Z angular rate in degrees/second */
} imu_bmi270_sample_t;

esp_err_t imu_bmi270_init(void);
esp_err_t imu_bmi270_read(imu_bmi270_sample_t *sample);
