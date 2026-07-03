#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/* Tab5 power/reset rails behind two PI4IOE5V6408 IO expanders on the internal
 * I2C bus. Pin roles (per m5stack/M5Tab5-UserDemo):
 *   expander 1 (0x43): P1 SPK_EN, P2 EXT5V_EN, P4 LCD_RST, P5 TP_RST, P6 CAM_RST
 *   expander 2 (0x44): P0 WLAN_PWR_EN, P3 USB5V_EN, P7 CHG_EN
 */

esp_err_t pi4ioe5v6408_board_init(void);
esp_err_t pi4ioe5v6408_set_output(uint8_t i2c_addr, uint8_t pin, bool level);
