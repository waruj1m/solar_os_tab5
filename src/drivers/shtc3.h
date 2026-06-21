#pragma once

#include <stdint.h>

#include "esp_err.h"

#define SHTC3_ADDRESS 0x70

typedef struct {
    float temperature_c;
    float humidity_percent;
    uint16_t id;
} shtc3_measurement_t;

esp_err_t shtc3_init(void);
esp_err_t shtc3_read_id(uint16_t *id);
esp_err_t shtc3_read_measurement(shtc3_measurement_t *measurement);
