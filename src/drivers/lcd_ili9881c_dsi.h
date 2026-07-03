#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_ldo_regulator.h"
#include "u8g2.h"

typedef struct {
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
    esp_err_t last_error;
} lcd_ili9881c_t;

esp_err_t lcd_ili9881c_init(lcd_ili9881c_t *display);
esp_err_t lcd_ili9881c_resume(lcd_ili9881c_t *display);
void lcd_ili9881c_deinit(lcd_ili9881c_t *display);
u8g2_t *lcd_ili9881c_get_u8g2(lcd_ili9881c_t *display);
