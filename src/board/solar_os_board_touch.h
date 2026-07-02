#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    bool touched;
    uint16_t x;
    uint16_t y;
    uint8_t gesture;
} solar_os_board_touch_point_t;

typedef struct {
    uint8_t point_count;
    solar_os_board_touch_point_t points[5];
} solar_os_board_touch_data_t;

esp_err_t solar_os_board_touch_init(void);
esp_err_t solar_os_board_touch_read(solar_os_board_touch_data_t *data);
bool solar_os_board_touch_available(void);
