#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define INA226_I2C_ADDRESS 0x40

typedef struct {
    uint16_t battery_mv;
    int16_t current_ma;
    uint16_t power_mw;
    bool calibrated;
} ina226_sample_t;

esp_err_t battery_ina226_init(void);
esp_err_t battery_ina226_read(ina226_sample_t *sample);
