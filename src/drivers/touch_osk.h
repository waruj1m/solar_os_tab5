#pragma once

#include "esp_err.h"
#include "lcd_ili9881c_dsi.h"

/* Capacitive touch + on-screen keyboard for boards without a physical
 * keyboard. Tries GT911 or ST7123 depending on which panel/touch pairing
 * lcd_ili9881c_dsi.c detected. Two-finger tap toggles the keyboard; taps on
 * it feed the normal solar_os_keyboard_read_chars() stream via
 * solar_os_keyboard_inject(). */
esp_err_t touch_osk_init(lcd_ili9881c_t *display);
