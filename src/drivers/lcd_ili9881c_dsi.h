#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_ldo_regulator.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "u8g2.h"

/* Tab5 units in the wild ship one of two panel/touch pairings; detected at
 * runtime (see lcd_panel_detect() in the .c file) the same way M5Stack's
 * own firmware does it. All three share the same 720x1280 resolution. */
typedef enum {
    LCD_PANEL_ILI9881C_GT911,
    LCD_PANEL_ST7121,
    LCD_PANEL_ST7123,
} lcd_panel_type_t;

typedef struct {
    lcd_panel_type_t panel_type;
    esp_lcd_dsi_bus_handle_t dsi_bus;
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_handle_t panel;
    esp_ldo_channel_handle_t phy_ldo;
    u8g2_t u8g2;
    uint8_t *buffer;
    uint8_t *shadow;
    uint16_t *line_buffer;
    size_t buffer_size;
    size_t line_buffer_size;
    /* Optional 1bpp overlay strip (same tile format as the main buffer,
     * full native height) composed over native x in [x0, x0+width). */
    const uint8_t *overlay;
    uint16_t overlay_x0;
    uint16_t overlay_width;
    SemaphoreHandle_t flush_mutex;
    esp_err_t last_error;
} lcd_ili9881c_t;

esp_err_t lcd_ili9881c_init(lcd_ili9881c_t *display);
esp_err_t lcd_ili9881c_resume(lcd_ili9881c_t *display);
void lcd_ili9881c_deinit(lcd_ili9881c_t *display);
u8g2_t *lcd_ili9881c_get_u8g2(lcd_ili9881c_t *display);
lcd_panel_type_t lcd_ili9881c_get_panel_type(const lcd_ili9881c_t *display);

/* Overlay control (used by the on-screen keyboard). Passing NULL disables
 * the overlay. Both calls repaint the whole panel. */
void lcd_ili9881c_set_overlay(lcd_ili9881c_t *display, const uint8_t *buffer,
                              uint16_t native_x0, uint16_t native_width);
esp_err_t lcd_ili9881c_flush_all(lcd_ili9881c_t *display);
