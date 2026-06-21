#pragma once

#include "driver/gpio.h"
#include "driver/i2s_types.h"
#include "driver/uart.h"

#define WS_RLCD_BOARD_NAME "Waveshare ESP32-S3-RLCD-4.2"
#define WS_RLCD_MODULE_NAME "ESP32-S3-WROOM-1-N16R8"

#define WS_RLCD_DISPLAY_CONTROLLER "ST7305"
#define WS_RLCD_DISPLAY_WIDTH 400
#define WS_RLCD_DISPLAY_HEIGHT 300

#define WS_RLCD_PIN_LCD_DC GPIO_NUM_5
#define WS_RLCD_PIN_LCD_CS GPIO_NUM_40
#define WS_RLCD_PIN_LCD_SCK GPIO_NUM_11
#define WS_RLCD_PIN_LCD_MOSI GPIO_NUM_12
#define WS_RLCD_PIN_LCD_RST GPIO_NUM_41
#define WS_RLCD_PIN_LCD_TE GPIO_NUM_6

#define WS_RLCD_PIN_I2C_SDA GPIO_NUM_13
#define WS_RLCD_PIN_I2C_SCL GPIO_NUM_14

#define WS_RLCD_PIN_SDMMC_CLK GPIO_NUM_38
#define WS_RLCD_PIN_SDMMC_CMD GPIO_NUM_21
#define WS_RLCD_PIN_SDMMC_D0 GPIO_NUM_39

#define WS_RLCD_PIN_BATTERY_ADC GPIO_NUM_4
#define WS_RLCD_BATTERY_ADC_DIVIDER_RATIO 3.0f

#define WS_RLCD_I2S_PORT I2S_NUM_0
#define WS_RLCD_PIN_I2S_MCLK GPIO_NUM_16
#define WS_RLCD_PIN_I2S_BCLK GPIO_NUM_9
#define WS_RLCD_PIN_I2S_WS GPIO_NUM_45
#define WS_RLCD_PIN_I2S_DIN GPIO_NUM_10
#define WS_RLCD_PIN_I2S_DOUT GPIO_NUM_8
#define WS_RLCD_PIN_AUDIO_PA GPIO_NUM_46
#define WS_RLCD_AUDIO_CODEC_OUT "ES8311"
#define WS_RLCD_AUDIO_CODEC_IN "ES7210"

#define WS_RLCD_PIN_KEY GPIO_NUM_18

#define WS_RLCD_EXPANSION_GPIO_MASK ((1ULL << GPIO_NUM_0) | \
                                     (1ULL << GPIO_NUM_1) | \
                                     (1ULL << GPIO_NUM_2) | \
                                     (1ULL << GPIO_NUM_3) | \
                                     (1ULL << GPIO_NUM_17) | \
                                     (1ULL << GPIO_NUM_18))
#define WS_RLCD_USER_GPIO_MASK ((1ULL << GPIO_NUM_1) | \
                                (1ULL << GPIO_NUM_2) | \
                                (1ULL << GPIO_NUM_3) | \
                                (1ULL << GPIO_NUM_17))

#define WS_RLCD_UART_PORT UART_NUM_0
#define WS_RLCD_PIN_UART_TX GPIO_NUM_43
#define WS_RLCD_PIN_UART_RX GPIO_NUM_44
