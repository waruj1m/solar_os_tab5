#pragma once

#include "driver/gpio.h"
#include "driver/i2c_types.h"
#include "driver/i2s_types.h"

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
     SOLAR_OS_BOARD_CAP_KEYBOARD | \
     SOLAR_OS_BOARD_CAP_WIFI | \
     SOLAR_OS_BOARD_CAP_SD | \
     SOLAR_OS_BOARD_CAP_RTC | \
     SOLAR_OS_BOARD_CAP_BATTERY | \
     SOLAR_OS_BOARD_CAP_AUDIO)

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

/* microSD on SDMMC slot 0 (IOMUX-fixed pins; the hosted ESP32-C6 owns
 * slot 1). The P4 SD IO rail is powered from on-chip LDO channel 4. */
#define SOLAR_OS_BOARD_SDMMC_SLOT 0
#define SOLAR_OS_BOARD_SDMMC_LDO_CHANNEL 4
#define SOLAR_OS_BOARD_PIN_SDMMC_CLK GPIO_NUM_43
#define SOLAR_OS_BOARD_PIN_SDMMC_CMD GPIO_NUM_44
#define SOLAR_OS_BOARD_PIN_SDMMC_D0 GPIO_NUM_39

/* RX8130CE RTC and INA226 battery monitor on the internal I2C bus. */
#define SOLAR_OS_BOARD_RTC_ADDR 0x32
#define SOLAR_OS_BOARD_BATTERY_MONITOR_ADDR 0x41

/* ES8388 DAC (speaker via NS4150B) + ES7210 dual-mic ADC on shared I2S.
 * The NS4150B enable (SPK_EN) sits on IO expander 1 and is raised during
 * board bring-up, so there is no PA GPIO on the SoC itself. */
#define SOLAR_OS_BOARD_AUDIO_OUT_ES8388 1
#define SOLAR_OS_BOARD_AUDIO_CODEC_OUT "ES8388"
#define SOLAR_OS_BOARD_AUDIO_CODEC_IN "ES7210"
#define SOLAR_OS_BOARD_I2S_PORT I2S_NUM_0
#define SOLAR_OS_BOARD_PIN_I2S_MCLK GPIO_NUM_30
#define SOLAR_OS_BOARD_PIN_I2S_BCLK GPIO_NUM_27
#define SOLAR_OS_BOARD_PIN_I2S_WS GPIO_NUM_29
#define SOLAR_OS_BOARD_PIN_I2S_DOUT GPIO_NUM_26
#define SOLAR_OS_BOARD_PIN_I2S_DIN GPIO_NUM_28
#define SOLAR_OS_BOARD_PIN_AUDIO_PA GPIO_NUM_NC

/* BMI270 6-axis IMU on the internal I2C bus (SDO tied low). No dedicated
 * capability/service exists yet -- see boards/drivers/imu_bmi270.cmake. */
#define SOLAR_OS_BOARD_IMU_ADDR 0x68

/* Internal I2C bus shared by GT911 touch (0x5D), RX8130 RTC (0x32),
 * INA226 power monitor (0x41), PI4IOE5V6408 IO expanders (0x43/0x44),
 * and the ES8388/ES7210 audio codecs. Pin map per m5stack/M5Tab5-UserDemo. */
#define SOLAR_OS_BOARD_I2C_PORT I2C_NUM_0
#define SOLAR_OS_BOARD_PIN_I2C_SDA GPIO_NUM_31
#define SOLAR_OS_BOARD_PIN_I2C_SCL GPIO_NUM_32
