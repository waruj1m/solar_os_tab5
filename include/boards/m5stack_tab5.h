#pragma once

#include "driver/gpio.h"
#include "driver/i2c_types.h"
#include "driver/i2s_types.h"
#include "driver/uart.h"
#include "sdmmc_cmd.h"

#define SOLAR_OS_BOARD_ID "m5stack_tab5"
#define SOLAR_OS_BOARD_NAME "M5Stack Tab5 (ESP32-P4)"
#define SOLAR_OS_BOARD_VENDOR "M5Stack"
#define SOLAR_OS_BOARD_MODULE_NAME "ESP32-P4NRW32"

#define SOLAR_OS_BOARD_CAPABILITIES \
    (SOLAR_OS_BOARD_CAP_PSRAM | \
     SOLAR_OS_BOARD_CAP_DISPLAY | \
     SOLAR_OS_BOARD_CAP_GFX | \
     SOLAR_OS_BOARD_CAP_CDC | \
     SOLAR_OS_BOARD_CAP_UART | \
     SOLAR_OS_BOARD_CAP_SD | \
     SOLAR_OS_BOARD_CAP_I2C | \
     SOLAR_OS_BOARD_CAP_RTC | \
     SOLAR_OS_BOARD_CAP_BATTERY | \
     SOLAR_OS_BOARD_CAP_AUDIO | \
     SOLAR_OS_BOARD_CAP_WIFI | \
     SOLAR_OS_BOARD_CAP_BLE | \
     SOLAR_OS_BOARD_CAP_GPIO | \
     SOLAR_OS_BOARD_CAP_KEY | \
     SOLAR_OS_BOARD_CAP_ADC)

/*
 * Display: 5" 1280x720 IPS MIPI-DSI
 * Controller: ST7123 or ILI9881C (auto-detected at runtime)
 * Touch: GT911 (I2C) or ST7123-integrated
 */
#define SOLAR_OS_BOARD_DISPLAY_CONTROLLER "MIPI-DSI"
#define SOLAR_OS_BOARD_DISPLAY_WIDTH 1280
#define SOLAR_OS_BOARD_DISPLAY_HEIGHT 720

#define SOLAR_OS_BOARD_PIN_LCD_BL GPIO_NUM_22
#define SOLAR_OS_BOARD_PIN_LCD_RST 255
#define SOLAR_OS_BOARD_PIN_LCD_MOSI -1
#define SOLAR_OS_BOARD_PIN_LCD_SCK -1
#define SOLAR_OS_BOARD_PIN_LCD_DC -1
#define SOLAR_OS_BOARD_PIN_LCD_CS -1
#define SOLAR_OS_BOARD_PIN_LCD_TE -1
#define SOLAR_OS_BOARD_PIN_TP_INT GPIO_NUM_23

/*
 * I2C bus 0 — internal bus shared by:
 *   GT911 touch    0x14 / 0x5D
 *   ST7123 touch   0x55
 *   ES8388 audio   0x10
 *   ES7210 audio   0x40 / 0x41
 *   BMI270 IMU     0x68
 *   RX8130CE RTC   0x32
 *   INA226 power   0x40 / 0x41
 *   PI4IOE5V6408   0x43 / 0x44
 */
#define SOLAR_OS_BOARD_I2C_PORT I2C_NUM_0
#define SOLAR_OS_BOARD_PIN_I2C_SDA GPIO_NUM_31
#define SOLAR_OS_BOARD_PIN_I2C_SCL GPIO_NUM_32

/*
 * Audio: ES8388 DAC + ES7210 AEC front-end
 * I2S port TDM shared between both codecs
 */
#define SOLAR_OS_BOARD_I2S_PORT 1
#define SOLAR_OS_BOARD_PIN_I2S_MCLK GPIO_NUM_30
#define SOLAR_OS_BOARD_PIN_I2S_BCLK GPIO_NUM_27
#define SOLAR_OS_BOARD_PIN_I2S_WS GPIO_NUM_29
#define SOLAR_OS_BOARD_PIN_I2S_DOUT GPIO_NUM_26
#define SOLAR_OS_BOARD_PIN_I2S_DIN GPIO_NUM_28
#define SOLAR_OS_BOARD_AUDIO_CODEC_OUT "ES8388"
#define SOLAR_OS_BOARD_AUDIO_CODEC_IN "ES7210"

/*
 * microSD (SDMMC 4-bit mode)
 */
#define SOLAR_OS_BOARD_PIN_SDMMC_CLK GPIO_NUM_43
#define SOLAR_OS_BOARD_PIN_SDMMC_CMD GPIO_NUM_44
#define SOLAR_OS_BOARD_PIN_SDMMC_D0 GPIO_NUM_39
#define SOLAR_OS_BOARD_PIN_SDMMC_D1 GPIO_NUM_40
#define SOLAR_OS_BOARD_PIN_SDMMC_D2 GPIO_NUM_41
#define SOLAR_OS_BOARD_PIN_SDMMC_D3 GPIO_NUM_42

/*
 * UART0 — RS-485 via SIT3088 transceiver
 * RS-485 DIR pin is used for half-duplex direction control.
 */
#define SOLAR_OS_BOARD_UART_PORT UART_NUM_0
#define SOLAR_OS_BOARD_PIN_UART_TX GPIO_NUM_20
#define SOLAR_OS_BOARD_PIN_UART_RX GPIO_NUM_21
#define SOLAR_OS_BOARD_PIN_UART_RS485_DIR GPIO_NUM_34

/*
 * BOOT button — used as KEY for sleep/wake/pairing
 */
#define SOLAR_OS_BOARD_PIN_KEY GPIO_NUM_35
#define SOLAR_OS_BOARD_KEY_ACTIVE_LEVEL 0
#define SOLAR_OS_BOARD_KEY_PULL_UP 1
#define SOLAR_OS_BOARD_KEY_PULL_DOWN 0

/*
 * Runtime-safe GPIOs (expansion headers, Grove, M5-Bus)
 * Exclude strapping pins (34-38), display, SD, I2C, audio, camera, C6 SDIO.
 */
#define SOLAR_OS_BOARD_EXPANSION_GPIO_MASK ((1ULL << GPIO_NUM_16) | \
                                            (1ULL << GPIO_NUM_17) | \
                                            (1ULL << GPIO_NUM_45) | \
                                            (1ULL << GPIO_NUM_52) | \
                                            (1ULL << GPIO_NUM_53) | \
                                            (1ULL << GPIO_NUM_54))
#define SOLAR_OS_BOARD_USER_GPIO_MASK ((1ULL << GPIO_NUM_16) | \
                                       (1ULL << GPIO_NUM_17) | \
                                       (1ULL << GPIO_NUM_52))
#define SOLAR_OS_BOARD_EXPANSION_GPIO_LIST "16 17 45 52 53 54"
#define SOLAR_OS_BOARD_USER_GPIO_LIST "16 17 52"
#define SOLAR_OS_BOARD_GPIO_SLOTS { \
    {.pin = 16, .runtime_allowed = true, .role = "expansion"}, \
    {.pin = 17, .runtime_allowed = true, .role = "expansion"}, \
    {.pin = 45, .runtime_allowed = false, .role = "expansion"}, \
    {.pin = 52, .runtime_allowed = true, .role = "expansion"}, \
    {.pin = 53, .runtime_allowed = false, .role = "Grove"}, \
    {.pin = 54, .runtime_allowed = false, .role = "Grove"}, \
}

/*
 * ADC — available on GPIO16-23 (ADC1) and GPIO49-54 (ADC2)
 */
#define SOLAR_OS_BOARD_ADC_MASK ((1ULL << GPIO_NUM_16) | \
                                 (1ULL << GPIO_NUM_17) | \
                                 (1ULL << GPIO_NUM_18) | \
                                 (1ULL << GPIO_NUM_19))
#define SOLAR_OS_BOARD_USER_ADC_LIST "16 17 18 19"
