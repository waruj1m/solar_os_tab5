#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "u8g2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_handle_t panel;
    u8g2_t u8g2;
    uint8_t *buffer;
    uint8_t *shadow;
    uint16_t *line_buffer;
    SemaphoreHandle_t trans_done;
    size_t buffer_size;
    size_t line_buffer_size;
    esp_err_t last_error;
    bool bus_initialized;
} lcd_st7789_t;

esp_err_t lcd_st7789_init(lcd_st7789_t *display);
esp_err_t lcd_st7789_resume(lcd_st7789_t *display);
void lcd_st7789_deinit(lcd_st7789_t *display);
u8g2_t *lcd_st7789_get_u8g2(lcd_st7789_t *display);

#ifdef __cplusplus
}
#endif
