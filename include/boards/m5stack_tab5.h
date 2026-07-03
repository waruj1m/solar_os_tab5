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
     SOLAR_OS_BOARD_CAP_I2C | \
     SOLAR_OS_BOARD_CAP_DISPLAY | \
     SOLAR_OS_BOARD_CAP_GFX | \
     SOLAR_OS_BOARD_CAP_KEYBOARD)

/* 5" 720x1280 portrait-native IPS behind MIPI-DSI (2 lanes). u8g2 renders
 * 1bpp at half resolution (360x640 native, scaled 2x2 in the blit driver);
 * the OS presents a 640x360 landscape canvas via the rotation offset. */
#define SOLAR_OS_BOARD_DISPLAY_CONTROLLER "ILI9881C"
#define SOLAR_OS_BOARD_DISPLAY_WIDTH 640
#define SOLAR_OS_BOARD_DISPLAY_HEIGHT 360
#define SOLAR_OS_BOARD_DISPLAY_SCALE 2
#define SOLAR_OS_BOARD_DISPLAY_ROTATION_OFFSET 1
#define SOLAR_OS_BOARD_PIN_LCD_BACKLIGHT GPIO_NUM_22

/* PI4IOE5V6408 IO expanders carrying power/reset rails (see pi4ioe5v6408.h). */
#define SOLAR_OS_BOARD_IO_EXPANDER1_ADDR 0x43
#define SOLAR_OS_BOARD_IO_EXPANDER2_ADDR 0x44

/* GT911 capacitive touch (original panel revision). */
#define SOLAR_OS_BOARD_PIN_TOUCH_INT GPIO_NUM_23

/* Internal I2C bus shared by GT911 touch (0x5D), RX8130 RTC (0x32),
 * INA226 power monitor (0x41), PI4IOE5V6408 IO expanders (0x43/0x44),
 * and the ES8388/ES7210 audio codecs. Pin map per m5stack/M5Tab5-UserDemo. */
#define SOLAR_OS_BOARD_I2C_PORT I2C_NUM_0
#define SOLAR_OS_BOARD_PIN_I2C_SDA GPIO_NUM_31
#define SOLAR_OS_BOARD_PIN_I2C_SCL GPIO_NUM_32
