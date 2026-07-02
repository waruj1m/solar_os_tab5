#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "u8g2.h"

typedef struct {
    void *dsi_bus;
    void *panel_io;
    void *panel;
    u8g2_t u8g2;
    uint8_t *fb;
    bool initialized;
} mipi_dsi_display_t;

esp_err_t mipi_dsi_display_init(mipi_dsi_display_t *display);
esp_err_t mipi_dsi_display_resume(mipi_dsi_display_t *display);
void mipi_dsi_display_deinit(mipi_dsi_display_t *display);
u8g2_t *mipi_dsi_get_u8g2(mipi_dsi_display_t *display);
void mipi_dsi_display_flush(mipi_dsi_display_t *display);
