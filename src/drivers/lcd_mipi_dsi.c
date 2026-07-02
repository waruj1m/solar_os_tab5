#include "lcd_mipi_dsi.h"

#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "lcd_mipi_dsi";

esp_err_t mipi_dsi_display_init(mipi_dsi_display_t *display)
{
    if (display == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(display, 0, sizeof(*display));
    ESP_LOGW(TAG, "MIPI-DSI init stubbed out for boot debugging");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t mipi_dsi_display_resume(mipi_dsi_display_t *display)
{
    return ESP_ERR_NOT_SUPPORTED;
}

void mipi_dsi_display_deinit(mipi_dsi_display_t *display)
{
}

u8g2_t *mipi_dsi_get_u8g2(mipi_dsi_display_t *display)
{
    return NULL;
}

void mipi_dsi_display_flush(mipi_dsi_display_t *display)
{
}
