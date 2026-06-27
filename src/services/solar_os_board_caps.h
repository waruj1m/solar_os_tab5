#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "solar_os_board.h"

typedef uint32_t solar_os_board_capabilities_t;

typedef enum {
    SOLAR_OS_BOARD_CAP_DISPLAY = 1U << 0,
    SOLAR_OS_BOARD_CAP_GFX = 1U << 1,
    SOLAR_OS_BOARD_CAP_CDC = 1U << 2,
    SOLAR_OS_BOARD_CAP_UART = 1U << 3,
    SOLAR_OS_BOARD_CAP_SD = 1U << 4,
    SOLAR_OS_BOARD_CAP_I2C = 1U << 5,
    SOLAR_OS_BOARD_CAP_RTC = 1U << 6,
    SOLAR_OS_BOARD_CAP_BATTERY = 1U << 7,
    SOLAR_OS_BOARD_CAP_AUDIO = 1U << 8,
    SOLAR_OS_BOARD_CAP_WIFI = 1U << 9,
    SOLAR_OS_BOARD_CAP_BLE = 1U << 10,
    SOLAR_OS_BOARD_CAP_GPIO = 1U << 11,
    SOLAR_OS_BOARD_CAP_ADC = 1U << 12,
    SOLAR_OS_BOARD_CAP_PWM = 1U << 13,
    SOLAR_OS_BOARD_CAP_KEY = 1U << 14,
    SOLAR_OS_BOARD_CAP_TEMPERATURE = 1U << 15,
    SOLAR_OS_BOARD_CAP_HUMIDITY = 1U << 16,
} solar_os_board_capability_t;

#ifndef SOLAR_OS_BOARD_CAPABILITIES
#define SOLAR_OS_BOARD_CAPABILITIES 0U
#endif

solar_os_board_capabilities_t solar_os_board_capabilities(void);
bool solar_os_board_has(solar_os_board_capability_t capability);
const char *solar_os_board_capability_name(solar_os_board_capability_t capability);
void solar_os_board_capabilities_format(char *buffer, size_t buffer_len);
