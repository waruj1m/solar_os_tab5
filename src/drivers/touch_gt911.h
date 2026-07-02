#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define GT911_I2C_ADDR_1 0x14
#define GT911_I2C_ADDR_2 0x5D

typedef struct {
    bool touched;
    uint16_t x;
    uint16_t y;
    uint16_t size;
} gt911_point_t;

typedef struct {
    uint8_t point_count;
    uint8_t gesture;
    gt911_point_t points[5];
} gt911_touch_data_t;

esp_err_t touch_gt911_init(void);
esp_err_t touch_gt911_read(gt911_touch_data_t *data);
