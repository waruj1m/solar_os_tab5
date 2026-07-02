#include "lcd_st7789.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "solar_os_board.h"

/* u8g2 keeps rendering 1bpp; each dirty tile row (8 pixel rows) is expanded
 * to RGB565 and pushed through esp_lcd. Panel driven in landscape via
 * swap_xy + mirror. Orientation/gap knobs below are bring-up tunables. */
#define LCD_SPI_CLOCK_HZ 40000000
#define LCD_TILE_WIDTH (SOLAR_OS_BOARD_DISPLAY_WIDTH / 8)
#define LCD_TILE_HEIGHT ((SOLAR_OS_BOARD_DISPLAY_HEIGHT + 7) / 8)
#define LCD_ROW_BYTES (LCD_TILE_WIDTH * 8)
#define LCD_SWAP_XY true
#define LCD_MIRROR_X true
#define LCD_MIRROR_Y false
#define LCD_INVERT_COLOR true

#define LCD_COLOR_FG 0xFFFFu /* white, byte-swapped for SPI below */
#define LCD_COLOR_BG 0x0000u

static const char *TAG = "lcd_st7789";
static lcd_st7789_t *active_display;

static const u8x8_display_info_t st7789_display_info = {
    .chip_enable_level = 0,
    .chip_disable_level = 1,
    .post_chip_enable_wait_ns = 0,
    .pre_chip_disable_wait_ns = 0,
    .reset_pulse_width_ms = 10,
    .post_reset_wait_ms = 120,
    .sda_setup_time_ns = 0,
    .sck_pulse_width_ns = 0,
    .sck_clock_hz = LCD_SPI_CLOCK_HZ,
    .spi_mode = 0,
    .i2c_bus_clock_100kHz = 4,
    .tile_width = LCD_TILE_WIDTH,
    .tile_height = LCD_TILE_HEIGHT,
    .default_x_offset = 0,
    .flipmode_x_offset = 0,
    .pixel_width = SOLAR_OS_BOARD_DISPLAY_WIDTH,
    .pixel_height = SOLAR_OS_BOARD_DISPLAY_HEIGHT,
};

static esp_err_t lcd_panel_setup(lcd_st7789_t *display)
{
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(display->panel), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(display->panel), TAG, "panel init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(display->panel, LCD_INVERT_COLOR), TAG,
                        "invert failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(display->panel, LCD_SWAP_XY), TAG,
                        "swap_xy failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(display->panel, LCD_MIRROR_X, LCD_MIRROR_Y), TAG,
                        "mirror failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(display->panel,
                                              SOLAR_OS_BOARD_DISPLAY_GAP_X,
                                              SOLAR_OS_BOARD_DISPLAY_GAP_Y),
                        TAG, "set_gap failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(display->panel, true), TAG,
                        "display on failed");
    return ESP_OK;
}

static void lcd_invalidate_shadow(lcd_st7789_t *display)
{
    if (display != NULL && display->shadow != NULL) {
        memset(display->shadow, 0xFF, display->buffer_size);
    }
}

static bool lcd_color_trans_done_cb(esp_lcd_panel_io_handle_t io,
                                    esp_lcd_panel_io_event_data_t *edata,
                                    void *user_ctx)
{
    (void)io;
    (void)edata;
    lcd_st7789_t *display = (lcd_st7789_t *)user_ctx;
    BaseType_t woken = pdFALSE;
    if (display->trans_done != NULL) {
        xSemaphoreGiveFromISR(display->trans_done, &woken);
    }
    return woken == pdTRUE;
}

static esp_err_t lcd_flush_tile_row(lcd_st7789_t *display, uint8_t y_pos)
{
    const int y0 = (int)y_pos * 8;
    int rows = 8;
    if (y0 + rows > SOLAR_OS_BOARD_DISPLAY_HEIGHT) {
        rows = SOLAR_OS_BOARD_DISPLAY_HEIGHT - y0;
    }
    if (rows <= 0) {
        return ESP_OK;
    }

    /* The previous draw_bitmap DMA may still be reading line_buffer. */
    if (display->trans_done != NULL) {
        xSemaphoreTake(display->trans_done, pdMS_TO_TICKS(250));
    }

    const uint8_t *src = display->buffer + (size_t)y_pos * LCD_ROW_BYTES;
    /* esp_lcd SPI sends bytes as-is; RGB565 needs byte swap. FG/BG are
     * symmetric (0xFFFF/0x0000) so no swap needed today. */
    for (int row = 0; row < rows; row++) {
        uint16_t *dest = display->line_buffer + (size_t)row * SOLAR_OS_BOARD_DISPLAY_WIDTH;
        const uint8_t mask = (uint8_t)(1U << row);
        for (int x = 0; x < SOLAR_OS_BOARD_DISPLAY_WIDTH; x++) {
            dest[x] = (src[x] & mask) ? LCD_COLOR_FG : LCD_COLOR_BG;
        }
    }

    return esp_lcd_panel_draw_bitmap(display->panel, 0, y0,
                                     SOLAR_OS_BOARD_DISPLAY_WIDTH, y0 + rows,
                                     display->line_buffer);
}

static uint8_t lcd_u8x8_byte_cb(u8x8_t *u8x8, uint8_t message, uint8_t arg_int, void *arg_ptr)
{
    (void)u8x8;
    (void)message;
    (void)arg_int;
    (void)arg_ptr;
    return 1;
}

static uint8_t lcd_u8x8_display_cb(u8x8_t *u8x8, uint8_t message, uint8_t arg_int, void *arg_ptr)
{
    if (message == U8X8_MSG_DISPLAY_SETUP_MEMORY) {
        u8x8_d_helper_display_setup_memory(u8x8, &st7789_display_info);
        return 1;
    }

    lcd_st7789_t *display = active_display;
    if (display == NULL) {
        return 0;
    }

    switch (message) {
    case U8X8_MSG_DISPLAY_INIT:
        return 1;

    case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
        if (esp_lcd_panel_disp_on_off(display->panel, arg_int == 0) != ESP_OK) {
            return 0;
        }
        if (arg_int == 0) {
            lcd_invalidate_shadow(display);
        }
        return 1;

    case U8X8_MSG_DISPLAY_DRAW_TILE: {
        const u8x8_tile_t *tile = (const u8x8_tile_t *)arg_ptr;
        const uint8_t y_pos = tile->y_pos;
        if (y_pos >= LCD_TILE_HEIGHT) {
            return 1;
        }

        const uint8_t *row = display->buffer + (size_t)y_pos * LCD_ROW_BYTES;
        if (display->shadow != NULL) {
            uint8_t *shadow_row = display->shadow + (size_t)y_pos * LCD_ROW_BYTES;
            if (memcmp(shadow_row, row, LCD_ROW_BYTES) == 0) {
                return 1;
            }
            memcpy(shadow_row, row, LCD_ROW_BYTES);
        }

        const esp_err_t err = lcd_flush_tile_row(display, y_pos);
        if (err != ESP_OK) {
            display->last_error = err;
            ESP_LOGE(TAG, "tile flush failed: %s", esp_err_to_name(err));
            return 0;
        }
        return 1;
    }

    default:
        return 0;
    }
}

static esp_err_t lcd_backlight_on(void)
{
    /* ponytail: full-on GPIO backlight; move to LEDC PWM if brightness
     * control is ever wanted. */
    const gpio_config_t io_config = {
        .pin_bit_mask = 1ULL << SOLAR_OS_BOARD_PIN_LCD_BACKLIGHT,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io_config), TAG, "backlight gpio config failed");
    return gpio_set_level(SOLAR_OS_BOARD_PIN_LCD_BACKLIGHT, 1);
}

esp_err_t lcd_st7789_init(lcd_st7789_t *display)
{
    if (display == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(display, 0, sizeof(*display));
    display->last_error = ESP_OK;

    const spi_bus_config_t bus_config = {
        .mosi_io_num = SOLAR_OS_BOARD_PIN_LCD_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = SOLAR_OS_BOARD_PIN_LCD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SOLAR_OS_BOARD_DISPLAY_WIDTH * 8 * (int)sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SOLAR_OS_BOARD_LCD_SPI_HOST, &bus_config,
                                           SPI_DMA_CH_AUTO),
                        TAG, "spi bus init failed");
    display->bus_initialized = true;

    display->trans_done = xSemaphoreCreateBinary();
    if (display->trans_done == NULL) {
        lcd_st7789_deinit(display);
        return ESP_ERR_NO_MEM;
    }
    xSemaphoreGive(display->trans_done);

    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = SOLAR_OS_BOARD_PIN_LCD_DC,
        .cs_gpio_num = SOLAR_OS_BOARD_PIN_LCD_CS,
        .pclk_hz = LCD_SPI_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 4,
        .on_color_trans_done = lcd_color_trans_done_cb,
        .user_ctx = display,
    };
    esp_err_t err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SOLAR_OS_BOARD_LCD_SPI_HOST,
                                             &io_config, &display->io);
    if (err != ESP_OK) {
        lcd_st7789_deinit(display);
        return err;
    }

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = SOLAR_OS_BOARD_PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    err = esp_lcd_new_panel_st7789(display->io, &panel_config, &display->panel);
    if (err != ESP_OK) {
        lcd_st7789_deinit(display);
        return err;
    }

    err = lcd_panel_setup(display);
    if (err != ESP_OK) {
        lcd_st7789_deinit(display);
        return err;
    }

    err = lcd_backlight_on();
    if (err != ESP_OK) {
        lcd_st7789_deinit(display);
        return err;
    }

    display->buffer_size = (size_t)LCD_ROW_BYTES * LCD_TILE_HEIGHT;
    display->buffer = heap_caps_malloc(display->buffer_size, MALLOC_CAP_8BIT);
    if (display->buffer == NULL) {
        lcd_st7789_deinit(display);
        return ESP_ERR_NO_MEM;
    }
    memset(display->buffer, 0, display->buffer_size);

    display->shadow = heap_caps_malloc(display->buffer_size, MALLOC_CAP_8BIT);
    if (display->shadow == NULL) {
        ESP_LOGW(TAG, "display shadow allocation failed, partial update skipping disabled");
    } else {
        lcd_invalidate_shadow(display);
    }

    display->line_buffer_size =
        (size_t)SOLAR_OS_BOARD_DISPLAY_WIDTH * 8 * sizeof(uint16_t);
    display->line_buffer = heap_caps_malloc(display->line_buffer_size,
                                            MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (display->line_buffer == NULL) {
        lcd_st7789_deinit(display);
        return ESP_ERR_NO_MEM;
    }

    u8g2_SetupDisplay(&display->u8g2, lcd_u8x8_display_cb, u8x8_dummy_cb,
                      lcd_u8x8_byte_cb, u8x8_dummy_cb);
    u8g2_SetupBuffer(&display->u8g2, display->buffer, LCD_TILE_HEIGHT,
                     u8g2_ll_hvline_vertical_top_lsb, U8G2_R0);
    active_display = display;
    u8g2_InitDisplay(&display->u8g2);
    u8g2_SetPowerSave(&display->u8g2, 0);

    return display->last_error;
}

esp_err_t lcd_st7789_resume(lcd_st7789_t *display)
{
    if (display == NULL || display->panel == NULL || display->buffer == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    active_display = display;
    display->last_error = ESP_OK;
    ESP_RETURN_ON_ERROR(lcd_panel_setup(display), TAG, "resume panel setup failed");
    ESP_RETURN_ON_ERROR(lcd_backlight_on(), TAG, "resume backlight failed");
    lcd_invalidate_shadow(display);
    u8g2_SetPowerSave(&display->u8g2, 0);
    return display->last_error;
}

void lcd_st7789_deinit(lcd_st7789_t *display)
{
    if (display == NULL) {
        return;
    }

    if (display->panel != NULL) {
        esp_lcd_panel_del(display->panel);
        display->panel = NULL;
    }
    if (display->io != NULL) {
        esp_lcd_panel_io_del(display->io);
        display->io = NULL;
    }
    if (display->bus_initialized) {
        spi_bus_free(SOLAR_OS_BOARD_LCD_SPI_HOST);
        display->bus_initialized = false;
    }

    if (display->buffer != NULL) {
        heap_caps_free(display->buffer);
        display->buffer = NULL;
    }
    if (display->shadow != NULL) {
        heap_caps_free(display->shadow);
        display->shadow = NULL;
    }
    if (display->line_buffer != NULL) {
        heap_caps_free(display->line_buffer);
        display->line_buffer = NULL;
    }
    if (display->trans_done != NULL) {
        vSemaphoreDelete(display->trans_done);
        display->trans_done = NULL;
    }

    if (active_display == display) {
        active_display = NULL;
    }

    display->buffer_size = 0;
    display->line_buffer_size = 0;
}

u8g2_t *lcd_st7789_get_u8g2(lcd_st7789_t *display)
{
    return display == NULL ? NULL : &display->u8g2;
}
