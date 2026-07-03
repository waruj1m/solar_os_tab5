#include "solar_os_board_display.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"
#include "lcd_ili9881c_dsi.h"
#include "pi4ioe5v6408.h"
#include "solar_os_board.h"
#include "touch_osk.h"

static const char *TAG = "board_display";
static lcd_ili9881c_t ili9881c_display;

static const char *panel_controller_name(lcd_panel_type_t type)
{
    switch (type) {
    case LCD_PANEL_ST7121:
        return "ST7121";
    case LCD_PANEL_ST7123:
        return "ST7123";
    case LCD_PANEL_ILI9881C_GT911:
    default:
        return "ILI9881C";
    }
}

static void display_bind_ili9881c(solar_os_board_display_t *display)
{
    display->driver = &ili9881c_display;
    display->u8g2 = lcd_ili9881c_get_u8g2(&ili9881c_display);
    display->controller = panel_controller_name(lcd_ili9881c_get_panel_type(&ili9881c_display));
    display->width = SOLAR_OS_BOARD_DISPLAY_WIDTH;
    display->height = SOLAR_OS_BOARD_DISPLAY_HEIGHT;
    display->ready = true;
}

esp_err_t solar_os_board_display_init(solar_os_board_display_t *display)
{
    if (display == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(display, 0, sizeof(*display));

    /* Panel and touch power/reset rails sit behind the IO expanders. */
    esp_err_t err = i2c_bus_init();
    if (err != ESP_OK) {
        return err;
    }
    err = pi4ioe5v6408_board_init();
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    /* lcd_ili9881c_init() detects which of the three panel/touch pairings
     * this unit has (see lcd_panel_detect() in lcd_ili9881c_dsi.c). */
    err = lcd_ili9881c_init(&ili9881c_display);
    if (err != ESP_OK) {
        return err;
    }

    /* Touch is part of the panel assembly; failure is non-fatal (the USB
     * keyboard still works without the on-screen keyboard). */
    if (touch_osk_init(&ili9881c_display) != ESP_OK) {
        ESP_LOGW(TAG, "touch/on-screen keyboard unavailable");
    }

    display_bind_ili9881c(display);
    return ESP_OK;
}

esp_err_t solar_os_board_display_resume(solar_os_board_display_t *display)
{
    if (display == NULL || display->driver == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    const esp_err_t err = lcd_ili9881c_resume((lcd_ili9881c_t *)display->driver);
    if (err != ESP_OK) {
        display->ready = false;
        return err;
    }

    display_bind_ili9881c(display);
    return ESP_OK;
}

void solar_os_board_display_deinit(solar_os_board_display_t *display)
{
    if (display != NULL && display->driver != NULL) {
        lcd_ili9881c_deinit((lcd_ili9881c_t *)display->driver);
        memset(display, 0, sizeof(*display));
    }
}

u8g2_t *solar_os_board_display_u8g2(solar_os_board_display_t *display)
{
    return display != NULL ? display->u8g2 : NULL;
}

const char *solar_os_board_display_controller(const solar_os_board_display_t *display)
{
    return display != NULL && display->controller != NULL ? display->controller : "unknown";
}

uint16_t solar_os_board_display_width(const solar_os_board_display_t *display)
{
    return display != NULL ? display->width : 0;
}

uint16_t solar_os_board_display_height(const solar_os_board_display_t *display)
{
    return display != NULL ? display->height : 0;
}

bool solar_os_board_display_ready(const solar_os_board_display_t *display)
{
    return display != NULL && display->ready;
}
