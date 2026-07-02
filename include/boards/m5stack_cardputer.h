#pragma once

#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "driver/uart.h"

#define SOLAR_OS_BOARD_ID "m5stack_cardputer"
#define SOLAR_OS_BOARD_NAME "M5Stack Cardputer"
#define SOLAR_OS_BOARD_VENDOR "M5Stack"
#define SOLAR_OS_BOARD_MODULE_NAME "M5StampS3 (ESP32-S3FN8)"

#define SOLAR_OS_BOARD_CAPABILITIES \
    (SOLAR_OS_BOARD_CAP_DISPLAY | \
     SOLAR_OS_BOARD_CAP_GFX | \
     SOLAR_OS_BOARD_CAP_CDC | \
     SOLAR_OS_BOARD_CAP_UART | \
     SOLAR_OS_BOARD_CAP_SD | \
     SOLAR_OS_BOARD_CAP_BATTERY | \
     SOLAR_OS_BOARD_CAP_WIFI | \
     SOLAR_OS_BOARD_CAP_GPIO | \
     SOLAR_OS_BOARD_CAP_ADC | \
     SOLAR_OS_BOARD_CAP_PWM | \
     SOLAR_OS_BOARD_CAP_KEY | \
     SOLAR_OS_BOARD_CAP_KEYBOARD)

/* ST7789V2 1.14" panel, native 135x240 portrait, driven landscape 240x135.
 * Offsets verified against M5GFX board_M5Cardputer (offset_x 52, offset_y 40
 * pre-rotation). Tune SOLAR_OS_BOARD_DISPLAY_GAP_* at bring-up if edges shift. */
#define SOLAR_OS_BOARD_DISPLAY_CONTROLLER "ST7789V2"
#define SOLAR_OS_BOARD_DISPLAY_WIDTH 240
#define SOLAR_OS_BOARD_DISPLAY_HEIGHT 135
#define SOLAR_OS_BOARD_DISPLAY_GAP_X 40
#define SOLAR_OS_BOARD_DISPLAY_GAP_Y 52

#define SOLAR_OS_BOARD_LCD_SPI_HOST SPI3_HOST
#define SOLAR_OS_BOARD_PIN_LCD_SCK GPIO_NUM_36
#define SOLAR_OS_BOARD_PIN_LCD_MOSI GPIO_NUM_35
#define SOLAR_OS_BOARD_PIN_LCD_DC GPIO_NUM_34
#define SOLAR_OS_BOARD_PIN_LCD_CS GPIO_NUM_37
#define SOLAR_OS_BOARD_PIN_LCD_RST GPIO_NUM_33
#define SOLAR_OS_BOARD_PIN_LCD_BACKLIGHT GPIO_NUM_38

/* Built-in 56-key matrix behind a 74HC138 demux. Pin map verified against
 * m5stack/M5Cardputer IOMatrixKeyboardReader. */
#define SOLAR_OS_BOARD_KEYBOARD_PIN_A0 GPIO_NUM_8
#define SOLAR_OS_BOARD_KEYBOARD_PIN_A1 GPIO_NUM_9
#define SOLAR_OS_BOARD_KEYBOARD_PIN_A2 GPIO_NUM_11
#define SOLAR_OS_BOARD_KEYBOARD_INPUT_PINS \
    {GPIO_NUM_13, GPIO_NUM_15, GPIO_NUM_3, GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7}
#define SOLAR_OS_BOARD_KEYBOARD_INPUT_COUNT 7

/* microSD wired for SPI mode only on this board. */
#define SOLAR_OS_BOARD_SD_USE_SPI 1
#define SOLAR_OS_BOARD_SD_SPI_HOST SPI2_HOST
#define SOLAR_OS_BOARD_PIN_SD_SCK GPIO_NUM_40
#define SOLAR_OS_BOARD_PIN_SD_MISO GPIO_NUM_39
#define SOLAR_OS_BOARD_PIN_SD_MOSI GPIO_NUM_14
#define SOLAR_OS_BOARD_PIN_SD_CS GPIO_NUM_12

/* ponytail: divider ratio is nominal (2x 100k); calibrate on hardware. */
#define SOLAR_OS_BOARD_PIN_BATTERY_ADC GPIO_NUM_10
#define SOLAR_OS_BOARD_BATTERY_ADC_DIVIDER_RATIO 2.0f

#define SOLAR_OS_BOARD_PIN_KEY GPIO_NUM_0
#define SOLAR_OS_BOARD_KEY_ACTIVE_LEVEL 0
#define SOLAR_OS_BOARD_KEY_PULL_UP 1
#define SOLAR_OS_BOARD_KEY_PULL_DOWN 0

/* Grove HY2.0-4P port (G1/G2). */
#define SOLAR_OS_BOARD_UART_PORT UART_NUM_1
#define SOLAR_OS_BOARD_PIN_UART_TX GPIO_NUM_2
#define SOLAR_OS_BOARD_PIN_UART_RX GPIO_NUM_1

#define SOLAR_OS_BOARD_EXPANSION_GPIO_MASK ((1ULL << GPIO_NUM_0) | \
                                            (1ULL << GPIO_NUM_1) | \
                                            (1ULL << GPIO_NUM_2))
#define SOLAR_OS_BOARD_USER_GPIO_MASK ((1ULL << GPIO_NUM_1) | \
                                       (1ULL << GPIO_NUM_2))
#define SOLAR_OS_BOARD_EXPANSION_GPIO_LIST "0 1 2"
#define SOLAR_OS_BOARD_USER_GPIO_LIST "1 2"
#define SOLAR_OS_BOARD_GPIO_SLOTS { \
    {.pin = 0, .runtime_allowed = false, .role = "BOOT/KEY"}, \
    {.pin = 1, .runtime_allowed = true, .role = "Grove"}, \
    {.pin = 2, .runtime_allowed = true, .role = "Grove"}, \
}
