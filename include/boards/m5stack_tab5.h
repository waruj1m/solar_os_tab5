#pragma once

#include "driver/gpio.h"
#include "driver/i2c_types.h"

#define SOLAR_OS_BOARD_ID "m5stack_tab5"
#define SOLAR_OS_BOARD_NAME "M5Stack Tab5"
#define SOLAR_OS_BOARD_VENDOR "M5Stack"
#define SOLAR_OS_BOARD_MODULE_NAME "ESP32-P4NRW32"

#define SOLAR_OS_BOARD_CAPABILITIES \
    (SOLAR_OS_BOARD_CAP_PSRAM | \
     SOLAR_OS_BOARD_CAP_CDC | \
     SOLAR_OS_BOARD_CAP_I2C)

/* Internal I2C bus shared by GT911 touch (0x5D), RX8130 RTC (0x32),
 * INA226 power monitor (0x41), PI4IOE5V6408 IO expanders (0x43/0x44),
 * and the ES8388/ES7210 audio codecs. Pin map per m5stack/M5Tab5-UserDemo. */
#define SOLAR_OS_BOARD_I2C_PORT I2C_NUM_0
#define SOLAR_OS_BOARD_PIN_I2C_SDA GPIO_NUM_31
#define SOLAR_OS_BOARD_PIN_I2C_SCL GPIO_NUM_32
