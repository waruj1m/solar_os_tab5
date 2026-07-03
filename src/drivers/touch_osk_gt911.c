#include "touch_osk_gt911.h"

#include <string.h>

#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"
#include "solar_os_board.h"
#include "solar_os_fonts.h"
#include "solar_os_keyboard.h"
#include "solar_os_keys.h"
#include "u8g2.h"

/* The OSK is a 1bpp overlay strip composed by the display driver over the
 * bottom of the landscape view. Under the terminal's default U8G2_R1
 * rotation, logical (lx, ly) maps to native (359 - ly, lx), so the bottom
 * OSK_HEIGHT logical rows are native x in [0, OSK_HEIGHT).
 *
 * Bring-up tunables: flip the touch mirrors if taps land mirrored, and
 * OSK_STRIP_AT_HIGH_X if the keyboard shows up at the top of the screen
 * (that would mean the terminal runs with rotation offset 3, not 1). */
#define OSK_TOUCH_MIRROR_X 0
#define OSK_TOUCH_MIRROR_Y 0
#define OSK_STRIP_AT_HIGH_X 0

#define OSK_PANEL_WIDTH 720
#define OSK_PANEL_HEIGHT 1280
#define OSK_SCALE SOLAR_OS_BOARD_DISPLAY_SCALE
#define OSK_NATIVE_WIDTH (OSK_PANEL_WIDTH / OSK_SCALE)   /* 360 */
#define OSK_NATIVE_HEIGHT (OSK_PANEL_HEIGHT / OSK_SCALE) /* 640 */
#define OSK_HEIGHT 160 /* logical rows; also the native strip width */
#define OSK_WIDTH OSK_NATIVE_HEIGHT /* logical columns (landscape width) */
#define OSK_TILE_HEIGHT (OSK_NATIVE_HEIGHT / 8)
#define OSK_BUFFER_SIZE ((size_t)OSK_HEIGHT * OSK_TILE_HEIGHT)

#define OSK_POLL_MS 33
#define OSK_ROWS 5
#define OSK_ROW_HEIGHT (OSK_HEIGHT / OSK_ROWS)
#define OSK_MAX_TOUCH_POINTS 5

/* Non-character key actions (outside the SOLAR_OS_KEY_* range). */
#define OSK_K_SHIFT 0xF8
#define OSK_K_CTRL 0xF9
#define OSK_K_HIDE 0xFA

typedef struct {
    const char *label;
    uint8_t base;
    uint8_t shifted;
    float width; /* row units; each row's units are normalized to OSK_WIDTH */
} osk_key_t;

static const osk_key_t osk_row0[] = {
    {"`", '`', '~', 1}, {"1", '1', '!', 1}, {"2", '2', '@', 1}, {"3", '3', '#', 1},
    {"4", '4', '$', 1}, {"5", '5', '%', 1}, {"6", '6', '^', 1}, {"7", '7', '&', 1},
    {"8", '8', '*', 1}, {"9", '9', '(', 1}, {"0", '0', ')', 1}, {"-", '-', '_', 1},
    {"=", '=', '+', 1}, {"bksp", '\b', '\b', 2},
};
static const osk_key_t osk_row1[] = {
    {"tab", '\t', '\t', 1.5f}, {"q", 'q', 'Q', 1}, {"w", 'w', 'W', 1}, {"e", 'e', 'E', 1},
    {"r", 'r', 'R', 1}, {"t", 't', 'T', 1}, {"y", 'y', 'Y', 1}, {"u", 'u', 'U', 1},
    {"i", 'i', 'I', 1}, {"o", 'o', 'O', 1}, {"p", 'p', 'P', 1}, {"[", '[', '{', 1},
    {"]", ']', '}', 1}, {"\\", '\\', '|', 1.5f},
};
static const osk_key_t osk_row2[] = {
    {"esc", SOLAR_OS_KEY_ESCAPE, SOLAR_OS_KEY_ESCAPE, 2}, {"a", 'a', 'A', 1},
    {"s", 's', 'S', 1}, {"d", 'd', 'D', 1}, {"f", 'f', 'F', 1}, {"g", 'g', 'G', 1},
    {"h", 'h', 'H', 1}, {"j", 'j', 'J', 1}, {"k", 'k', 'K', 1}, {"l", 'l', 'L', 1},
    {";", ';', ':', 1}, {"'", '\'', '"', 1}, {"enter", '\n', '\n', 2},
};
static const osk_key_t osk_row3[] = {
    {"shift", OSK_K_SHIFT, OSK_K_SHIFT, 2.5f}, {"z", 'z', 'Z', 1}, {"x", 'x', 'X', 1},
    {"c", 'c', 'C', 1}, {"v", 'v', 'V', 1}, {"b", 'b', 'B', 1}, {"n", 'n', 'N', 1},
    {"m", 'm', 'M', 1}, {",", ',', '<', 1}, {".", '.', '>', 1}, {"/", '/', '?', 1},
    {"up", SOLAR_OS_KEY_UP, SOLAR_OS_KEY_UP, 1},
    {"del", SOLAR_OS_KEY_DELETE, SOLAR_OS_KEY_DELETE, 1.5f},
};
static const osk_key_t osk_row4[] = {
    {"ctrl", OSK_K_CTRL, OSK_K_CTRL, 2}, {"hide", OSK_K_HIDE, OSK_K_HIDE, 2},
    {"space", ' ', ' ', 8},
    {"lt", SOLAR_OS_KEY_LEFT, SOLAR_OS_KEY_LEFT, 1},
    {"dn", SOLAR_OS_KEY_DOWN, SOLAR_OS_KEY_DOWN, 1},
    {"rt", SOLAR_OS_KEY_RIGHT, SOLAR_OS_KEY_RIGHT, 1},
};

typedef struct {
    const osk_key_t *keys;
    size_t count;
} osk_row_t;

static const osk_row_t osk_rows[OSK_ROWS] = {
    {osk_row0, sizeof(osk_row0) / sizeof(osk_row0[0])},
    {osk_row1, sizeof(osk_row1) / sizeof(osk_row1[0])},
    {osk_row2, sizeof(osk_row2) / sizeof(osk_row2[0])},
    {osk_row3, sizeof(osk_row3) / sizeof(osk_row3[0])},
    {osk_row4, sizeof(osk_row4) / sizeof(osk_row4[0])},
};

static const char *TAG = "touch_osk";

static lcd_ili9881c_t *osk_display;
static esp_lcd_touch_handle_t touch_handle;
static u8g2_t osk_u8g2;
static uint8_t osk_buffer[OSK_BUFFER_SIZE];
static bool osk_visible;
static bool osk_shift;
static bool osk_ctrl;
static bool osk_was_pressed;

static const u8x8_display_info_t osk_display_info = {
    .tile_width = OSK_HEIGHT / 8,
    .tile_height = OSK_TILE_HEIGHT,
    .pixel_width = OSK_HEIGHT,
    .pixel_height = OSK_NATIVE_HEIGHT,
};

static uint8_t osk_u8x8_cb(u8x8_t *u8x8, uint8_t message, uint8_t arg_int, void *arg_ptr)
{
    (void)arg_int;
    (void)arg_ptr;
    if (message == U8X8_MSG_DISPLAY_SETUP_MEMORY) {
        u8x8_d_helper_display_setup_memory(u8x8, &osk_display_info);
    }
    /* Memory-only canvas: the display driver composes the buffer itself. */
    return 1;
}

static uint16_t osk_row_units_to_px(const osk_row_t *row, float units)
{
    float total = 0;
    for (size_t i = 0; i < row->count; i++) {
        total += row->keys[i].width;
    }
    return (uint16_t)((units / total) * OSK_WIDTH);
}

static void osk_draw(void)
{
    u8g2_ClearBuffer(&osk_u8g2);
    u8g2_SetFont(&osk_u8g2, u8g2_font_solar_os_default_r_14_tf);

    for (size_t r = 0; r < OSK_ROWS; r++) {
        const osk_row_t *row = &osk_rows[r];
        const u8g2_uint_t y = (u8g2_uint_t)(r * OSK_ROW_HEIGHT);
        float x_units = 0;
        for (size_t k = 0; k < row->count; k++) {
            const osk_key_t *key = &row->keys[k];
            const uint16_t x0 = osk_row_units_to_px(row, x_units);
            const uint16_t x1 = osk_row_units_to_px(row, x_units + key->width);
            const uint16_t w = (uint16_t)(x1 - x0);
            const bool active = (key->base == OSK_K_SHIFT && osk_shift) ||
                                (key->base == OSK_K_CTRL && osk_ctrl);

            if (active) {
                u8g2_DrawBox(&osk_u8g2, (u8g2_uint_t)x0, y, w, OSK_ROW_HEIGHT);
                u8g2_SetDrawColor(&osk_u8g2, 0);
            } else {
                u8g2_DrawFrame(&osk_u8g2, (u8g2_uint_t)x0, y, w, OSK_ROW_HEIGHT);
            }

            const char *label = key->label;
            char shifted_label[2];
            if (osk_shift && key->shifted >= 0x20 && key->shifted < 0x7f &&
                key->shifted != key->base) {
                shifted_label[0] = (char)key->shifted;
                shifted_label[1] = '\0';
                label = shifted_label;
            }
            const u8g2_uint_t label_w = u8g2_GetStrWidth(&osk_u8g2, label);
            u8g2_DrawStr(&osk_u8g2,
                         (u8g2_uint_t)(x0 + (w > label_w ? (w - label_w) / 2 : 1)),
                         (u8g2_uint_t)(y + OSK_ROW_HEIGHT - 9),
                         label);
            if (active) {
                u8g2_SetDrawColor(&osk_u8g2, 1);
            }
            x_units += key->width;
        }
    }
}

static void osk_show(bool show)
{
    osk_visible = show;
    osk_shift = false;
    osk_ctrl = false;
    if (show) {
        osk_draw();
        lcd_ili9881c_set_overlay(osk_display, osk_buffer,
                                 OSK_STRIP_AT_HIGH_X ? OSK_NATIVE_WIDTH - OSK_HEIGHT : 0,
                                 OSK_HEIGHT);
    } else {
        lcd_ili9881c_set_overlay(osk_display, NULL, 0, 0);
    }
}

static void osk_refresh(void)
{
    osk_draw();
    (void)lcd_ili9881c_flush_all(osk_display);
}

static void osk_press_key(const osk_key_t *key)
{
    switch (key->base) {
    case OSK_K_SHIFT:
        osk_shift = !osk_shift;
        osk_refresh();
        return;
    case OSK_K_CTRL:
        osk_ctrl = !osk_ctrl;
        osk_refresh();
        return;
    case OSK_K_HIDE:
        osk_show(false);
        return;
    default:
        break;
    }

    char ch = (char)(osk_shift ? key->shifted : key->base);
    if (osk_ctrl && key->base >= 'a' && key->base <= 'z') {
        ch = (char)(key->base - 'a' + 1);
    }
    (void)solar_os_keyboard_inject(&ch, 1);

    if (osk_shift || osk_ctrl) {
        osk_shift = false;
        osk_ctrl = false;
        osk_refresh();
    }
}

static void osk_handle_tap(uint16_t panel_x, uint16_t panel_y)
{
    /* Panel coords are native coords x OSK_SCALE; logical under R1:
     * lx = native_y, ly = (native_width - 1) - native_x. */
    uint16_t nx = (uint16_t)(panel_x / OSK_SCALE);
    uint16_t ny = (uint16_t)(panel_y / OSK_SCALE);
#if OSK_TOUCH_MIRROR_X
    nx = (uint16_t)(OSK_NATIVE_WIDTH - 1 - nx);
#endif
#if OSK_TOUCH_MIRROR_Y
    ny = (uint16_t)(OSK_NATIVE_HEIGHT - 1 - ny);
#endif
    const uint16_t lx = ny;
    const uint16_t ly = (uint16_t)(OSK_NATIVE_WIDTH - 1 - nx);

    const uint16_t osk_top = (uint16_t)(OSK_NATIVE_WIDTH - OSK_HEIGHT);
    if (lx >= OSK_WIDTH || ly < osk_top) {
        return; /* outside the keyboard strip */
    }

    const uint16_t oy = (uint16_t)(ly - osk_top);
    size_t r = oy / OSK_ROW_HEIGHT;
    if (r >= OSK_ROWS) {
        r = OSK_ROWS - 1;
    }
    const osk_row_t *row = &osk_rows[r];
    float x_units = 0;
    for (size_t k = 0; k < row->count; k++) {
        const uint16_t x0 = osk_row_units_to_px(row, x_units);
        const uint16_t x1 = osk_row_units_to_px(row, x_units + row->keys[k].width);
        if (lx >= x0 && lx < x1) {
            osk_press_key(&row->keys[k]);
            return;
        }
        x_units += row->keys[k].width;
    }
}

static void osk_touch_task(void *arg)
{
    (void)arg;

    uint16_t x[OSK_MAX_TOUCH_POINTS];
    uint16_t y[OSK_MAX_TOUCH_POINTS];
    uint8_t count = 0;

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(OSK_POLL_MS));

        i2c_bus_lock();
        const esp_err_t err = esp_lcd_touch_read_data(touch_handle);
        bool pressed = false;
        if (err == ESP_OK) {
            pressed = esp_lcd_touch_get_coordinates(touch_handle, x, y, NULL,
                                                    &count, OSK_MAX_TOUCH_POINTS);
        }
        i2c_bus_unlock();

        if (!pressed || count == 0) {
            osk_was_pressed = false;
            continue;
        }
        if (osk_was_pressed) {
            continue; /* act on press transitions only */
        }
        osk_was_pressed = true;

        if (count >= 2) {
            osk_show(!osk_visible);
        } else if (osk_visible) {
            osk_handle_tap(x[0], y[0]);
        }
    }
}

esp_err_t touch_osk_gt911_init(lcd_ili9881c_t *display)
{
    if (display == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (touch_handle != NULL) {
        return ESP_OK;
    }

    osk_display = display;

    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    io_config.scl_speed_hz = i2c_bus_get_speed_hz();
    esp_lcd_panel_io_handle_t io = NULL;
    esp_err_t err = esp_lcd_new_panel_io_i2c(i2c_bus_get_handle(), &io_config, &io);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "touch io init failed: %s", esp_err_to_name(err));
        return err;
    }

    const esp_lcd_touch_config_t touch_config = {
        .x_max = OSK_PANEL_WIDTH,
        .y_max = OSK_PANEL_HEIGHT,
        .rst_gpio_num = -1, /* reset wired to the IO expander */
        .int_gpio_num = -1, /* polled; INT is GPIO 23 if IRQs are ever wanted */
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
    };
    err = esp_lcd_touch_new_i2c_gt911(io, &touch_config, &touch_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "GT911 init failed: %s", esp_err_to_name(err));
        esp_lcd_panel_io_del(io);
        return err;
    }

    u8g2_SetupDisplay(&osk_u8g2, osk_u8x8_cb, u8x8_dummy_cb, osk_u8x8_cb, u8x8_dummy_cb);
    u8g2_SetupBuffer(&osk_u8g2, osk_buffer, OSK_TILE_HEIGHT,
                     u8g2_ll_hvline_vertical_top_lsb, U8G2_R1);
    u8g2_InitDisplay(&osk_u8g2);

    if (xTaskCreate(osk_touch_task, "touch_osk", 4096, NULL, 4, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "GT911 touch + on-screen keyboard ready (two-finger tap toggles)");
    return ESP_OK;
}
