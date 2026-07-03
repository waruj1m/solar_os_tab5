#include "lcd_ili9881c_dsi.h"

#include <string.h>

#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_ili9881c.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "solar_os_board.h"

#include "lcd_ili9881c_init_data.inc"

/* u8g2 keeps rendering 1bpp, at 1/SCALE of the panel resolution and in the
 * panel-native portrait orientation (the terminal presents landscape via
 * SOLAR_OS_BOARD_DISPLAY_ROTATION_OFFSET). Each dirty tile row (8 u8g2 pixel
 * rows) is expanded SCALExSCALE to RGB565 and copied into the MIPI-DSI DPI
 * framebuffer. DSI timing constants below come from m5stack/M5Tab5-UserDemo
 * and are bring-up tunables. */
#define LCD_PANEL_WIDTH 720
#define LCD_PANEL_HEIGHT 1280
#define LCD_SCALE SOLAR_OS_BOARD_DISPLAY_SCALE
#define LCD_NATIVE_WIDTH (LCD_PANEL_WIDTH / LCD_SCALE)
#define LCD_NATIVE_HEIGHT (LCD_PANEL_HEIGHT / LCD_SCALE)
#define LCD_TILE_WIDTH (LCD_NATIVE_WIDTH / 8)
#define LCD_TILE_HEIGHT ((LCD_NATIVE_HEIGHT + 7) / 8)
#define LCD_ROW_BYTES (LCD_TILE_WIDTH * 8)

#define LCD_DSI_LANES 2
#define LCD_DSI_LANE_BIT_RATE_MBPS 730
#define LCD_DPI_CLOCK_MHZ 60
#define LCD_DSI_PHY_LDO_CHANNEL 3
#define LCD_DSI_PHY_LDO_VOLTAGE_MV 2500

#define LCD_BACKLIGHT_LEDC_TIMER LEDC_TIMER_0
#define LCD_BACKLIGHT_LEDC_CHANNEL LEDC_CHANNEL_1
#define LCD_BACKLIGHT_LEDC_FREQ_HZ 5000
#define LCD_BACKLIGHT_LEDC_DUTY_RES LEDC_TIMER_12_BIT
#define LCD_BACKLIGHT_LEDC_DUTY_MAX 4095

#define LCD_COLOR_FG 0xFFFFu
#define LCD_COLOR_BG 0x0000u

/* Board macros describe the logical landscape canvas; the u8g2 buffer is the
 * portrait-native transpose of it. */
_Static_assert(LCD_NATIVE_WIDTH == SOLAR_OS_BOARD_DISPLAY_HEIGHT,
               "board display height must be panel width / scale");
_Static_assert(LCD_NATIVE_HEIGHT == SOLAR_OS_BOARD_DISPLAY_WIDTH,
               "board display width must be panel height / scale");
_Static_assert(LCD_NATIVE_WIDTH % 8 == 0, "native width must be tile aligned");

static const char *TAG = "lcd_ili9881c";
static lcd_ili9881c_t *active_display;

static const u8x8_display_info_t ili9881c_display_info = {
    .chip_enable_level = 0,
    .chip_disable_level = 1,
    .post_chip_enable_wait_ns = 0,
    .pre_chip_disable_wait_ns = 0,
    .reset_pulse_width_ms = 10,
    .post_reset_wait_ms = 120,
    .sda_setup_time_ns = 0,
    .sck_pulse_width_ns = 0,
    .sck_clock_hz = 0,
    .spi_mode = 0,
    .i2c_bus_clock_100kHz = 4,
    .tile_width = LCD_TILE_WIDTH,
    .tile_height = LCD_TILE_HEIGHT,
    .default_x_offset = 0,
    .flipmode_x_offset = 0,
    .pixel_width = LCD_NATIVE_WIDTH,
    .pixel_height = LCD_NATIVE_HEIGHT,
};

static esp_err_t lcd_backlight_set(uint32_t duty)
{
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_BACKLIGHT_LEDC_CHANNEL, duty),
                        TAG, "backlight duty failed");
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_BACKLIGHT_LEDC_CHANNEL);
}

static esp_err_t lcd_backlight_init(void)
{
    const ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LCD_BACKLIGHT_LEDC_DUTY_RES,
        .timer_num = LCD_BACKLIGHT_LEDC_TIMER,
        .freq_hz = LCD_BACKLIGHT_LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_config), TAG, "backlight timer failed");

    const ledc_channel_config_t channel_config = {
        .gpio_num = SOLAR_OS_BOARD_PIN_LCD_BACKLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LCD_BACKLIGHT_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LCD_BACKLIGHT_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel_config), TAG, "backlight channel failed");
    return lcd_backlight_set(LCD_BACKLIGHT_LEDC_DUTY_MAX);
}

static void lcd_invalidate_shadow(lcd_ili9881c_t *display)
{
    if (display != NULL && display->shadow != NULL) {
        memset(display->shadow, 0xFF, display->buffer_size);
    }
}

static esp_err_t lcd_flush_tile_row(lcd_ili9881c_t *display, uint8_t y_pos)
{
    const int y0 = (int)y_pos * 8;
    int rows = 8;
    if (y0 + rows > LCD_NATIVE_HEIGHT) {
        rows = LCD_NATIVE_HEIGHT - y0;
    }
    if (rows <= 0) {
        return ESP_OK;
    }

    const uint8_t *src = display->buffer + (size_t)y_pos * LCD_ROW_BYTES;
    for (int row = 0; row < rows; row++) {
        uint16_t *line = display->line_buffer + (size_t)row * LCD_SCALE * LCD_PANEL_WIDTH;
        const uint8_t mask = (uint8_t)(1U << row);
        for (int x = 0; x < LCD_NATIVE_WIDTH; x++) {
            const uint16_t color = (src[x] & mask) ? LCD_COLOR_FG : LCD_COLOR_BG;
            for (int sx = 0; sx < LCD_SCALE; sx++) {
                line[x * LCD_SCALE + sx] = color;
            }
        }
        for (int sy = 1; sy < LCD_SCALE; sy++) {
            memcpy(line + (size_t)sy * LCD_PANEL_WIDTH, line,
                   (size_t)LCD_PANEL_WIDTH * sizeof(uint16_t));
        }
    }

    /* DPI draw_bitmap copies into the panel framebuffer synchronously. */
    return esp_lcd_panel_draw_bitmap(display->panel, 0, y0 * LCD_SCALE,
                                     LCD_PANEL_WIDTH, (y0 + rows) * LCD_SCALE,
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
        u8x8_d_helper_display_setup_memory(u8x8, &ili9881c_display_info);
        return 1;
    }

    lcd_ili9881c_t *display = active_display;
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
        lcd_backlight_set(arg_int == 0 ? LCD_BACKLIGHT_LEDC_DUTY_MAX : 0);
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

static esp_err_t lcd_panel_setup(lcd_ili9881c_t *display)
{
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(display->panel), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(display->panel), TAG, "panel init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(display->panel, true), TAG,
                        "display on failed");
    return ESP_OK;
}

esp_err_t lcd_ili9881c_init(lcd_ili9881c_t *display)
{
    if (display == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(display, 0, sizeof(*display));
    display->last_error = ESP_OK;

    /* VDD_MIPI_DPHY is fed from the on-chip LDO on the Tab5. */
    const esp_ldo_channel_config_t ldo_config = {
        .chan_id = LCD_DSI_PHY_LDO_CHANNEL,
        .voltage_mv = LCD_DSI_PHY_LDO_VOLTAGE_MV,
    };
    esp_err_t err = esp_ldo_acquire_channel(&ldo_config, &display->phy_ldo);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "DSI PHY LDO acquire failed: %s", esp_err_to_name(err));
        return err;
    }

    const esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id = 0,
        .num_data_lanes = LCD_DSI_LANES,
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = LCD_DSI_LANE_BIT_RATE_MBPS,
    };
    err = esp_lcd_new_dsi_bus(&bus_config, &display->dsi_bus);
    if (err != ESP_OK) {
        lcd_ili9881c_deinit(display);
        return err;
    }

    const esp_lcd_dbi_io_config_t dbi_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    err = esp_lcd_new_panel_io_dbi(display->dsi_bus, &dbi_config, &display->io);
    if (err != ESP_OK) {
        lcd_ili9881c_deinit(display);
        return err;
    }

    esp_lcd_dpi_panel_config_t dpi_config = {
        .virtual_channel = 0,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = LCD_DPI_CLOCK_MHZ,
        .pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565,
        .num_fbs = 1,
        .video_timing = {
            .h_size = LCD_PANEL_WIDTH,
            .v_size = LCD_PANEL_HEIGHT,
            .hsync_back_porch = 140,
            .hsync_pulse_width = 40,
            .hsync_front_porch = 40,
            .vsync_back_porch = 20,
            .vsync_pulse_width = 4,
            .vsync_front_porch = 20,
        },
        /* ponytail: dirty bands are CPU memcpy'd; enable DMA2D if terminal
         * scrolling ever feels slow. */
        .flags.use_dma2d = false,
    };

    ili9881c_vendor_config_t vendor_config = {
        .init_cmds = tab5_lcd_ili9881c_specific_init_code_default,
        .init_cmds_size = sizeof(tab5_lcd_ili9881c_specific_init_code_default) /
                          sizeof(tab5_lcd_ili9881c_specific_init_code_default[0]),
        .mipi_config = {
            .dsi_bus = display->dsi_bus,
            .dpi_config = &dpi_config,
            .lane_num = LCD_DSI_LANES,
        },
    };

    const esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 16,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .reset_gpio_num = -1, /* reset wired to the PI4IOE5V6408 expander */
        .vendor_config = &vendor_config,
    };
    err = esp_lcd_new_panel_ili9881c(display->io, &panel_config, &display->panel);
    if (err != ESP_OK) {
        lcd_ili9881c_deinit(display);
        return err;
    }

    err = lcd_panel_setup(display);
    if (err != ESP_OK) {
        lcd_ili9881c_deinit(display);
        return err;
    }

    err = lcd_backlight_init();
    if (err != ESP_OK) {
        lcd_ili9881c_deinit(display);
        return err;
    }

    display->buffer_size = (size_t)LCD_ROW_BYTES * LCD_TILE_HEIGHT;
    display->buffer = heap_caps_malloc(display->buffer_size, MALLOC_CAP_8BIT);
    if (display->buffer == NULL) {
        lcd_ili9881c_deinit(display);
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
        (size_t)LCD_PANEL_WIDTH * 8 * LCD_SCALE * sizeof(uint16_t);
    display->line_buffer = heap_caps_malloc(display->line_buffer_size, MALLOC_CAP_8BIT);
    if (display->line_buffer == NULL) {
        lcd_ili9881c_deinit(display);
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

esp_err_t lcd_ili9881c_resume(lcd_ili9881c_t *display)
{
    if (display == NULL || display->panel == NULL || display->buffer == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    active_display = display;
    display->last_error = ESP_OK;
    ESP_RETURN_ON_ERROR(lcd_panel_setup(display), TAG, "resume panel setup failed");
    ESP_RETURN_ON_ERROR(lcd_backlight_set(LCD_BACKLIGHT_LEDC_DUTY_MAX), TAG,
                        "resume backlight failed");
    lcd_invalidate_shadow(display);
    u8g2_SetPowerSave(&display->u8g2, 0);
    return display->last_error;
}

void lcd_ili9881c_deinit(lcd_ili9881c_t *display)
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
    if (display->dsi_bus != NULL) {
        esp_lcd_del_dsi_bus(display->dsi_bus);
        display->dsi_bus = NULL;
    }
    if (display->phy_ldo != NULL) {
        esp_ldo_release_channel(display->phy_ldo);
        display->phy_ldo = NULL;
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

    if (active_display == display) {
        active_display = NULL;
    }

    display->buffer_size = 0;
    display->line_buffer_size = 0;
}

u8g2_t *lcd_ili9881c_get_u8g2(lcd_ili9881c_t *display)
{
    return display == NULL ? NULL : &display->u8g2;
}
