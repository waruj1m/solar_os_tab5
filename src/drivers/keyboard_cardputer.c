#include "solar_os_keyboard.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "solar_os_board.h"
#include "solar_os_keys.h"

/* Cardputer 56-key matrix behind a 74HC138 demux. Scan transform and key map
 * mirror m5stack/M5Cardputer's IOMatrixKeyboardReader / Keyboard.h. */

#define KB_ROWS 4
#define KB_COLS 14
#define KB_KEY_COUNT (KB_ROWS * KB_COLS)
#define KB_SCAN_PERIOD_MS 10
#define KB_QUEUE_LEN 64
#define KB_REPEAT_RATE_CPS 15U
#define KB_REPEAT_DELAY_MS 450U
#define KB_SEQUENCE_MAX 2

#define KB_KEY_NONE 0
#define KB_KEY_FN 1
#define KB_KEY_SHIFT 2
#define KB_KEY_CTRL 3
#define KB_KEY_OPT 4
#define KB_KEY_ALT 5

typedef struct {
    char base;
    char shifted;
    uint8_t fn; /* SOLAR_OS_KEY_* / char for the Fn layer, KB_KEY_NONE if none */
    uint8_t modifier; /* KB_KEY_* modifier id, or KB_KEY_NONE */
} kb_key_def_t;

/* Row 0 is the top (number) row, column 0 leftmost, matching the legend. */
static const kb_key_def_t key_map[KB_ROWS][KB_COLS] = {
    {
        {'`', '~', SOLAR_OS_KEY_ESCAPE, KB_KEY_NONE},
        {'1', '!', SOLAR_OS_KEY_F1, KB_KEY_NONE},
        {'2', '@', SOLAR_OS_KEY_F2, KB_KEY_NONE},
        {'3', '#', SOLAR_OS_KEY_F3, KB_KEY_NONE},
        {'4', '$', SOLAR_OS_KEY_F4, KB_KEY_NONE},
        {'5', '%', SOLAR_OS_KEY_F5, KB_KEY_NONE},
        {'6', '^', SOLAR_OS_KEY_F6, KB_KEY_NONE},
        {'7', '&', SOLAR_OS_KEY_F7, KB_KEY_NONE},
        {'8', '*', SOLAR_OS_KEY_F8, KB_KEY_NONE},
        {'9', '(', SOLAR_OS_KEY_F9, KB_KEY_NONE},
        {'0', ')', SOLAR_OS_KEY_F10, KB_KEY_NONE},
        {'-', '_', SOLAR_OS_KEY_F11, KB_KEY_NONE},
        {'=', '+', SOLAR_OS_KEY_F12, KB_KEY_NONE},
        {'\b', '\b', SOLAR_OS_KEY_DELETE, KB_KEY_NONE},
    },
    {
        {'\t', '\t', KB_KEY_NONE, KB_KEY_NONE},
        {'q', 'Q', KB_KEY_NONE, KB_KEY_NONE},
        {'w', 'W', KB_KEY_NONE, KB_KEY_NONE},
        {'e', 'E', KB_KEY_NONE, KB_KEY_NONE},
        {'r', 'R', KB_KEY_NONE, KB_KEY_NONE},
        {'t', 'T', KB_KEY_NONE, KB_KEY_NONE},
        {'y', 'Y', KB_KEY_NONE, KB_KEY_NONE},
        {'u', 'U', KB_KEY_NONE, KB_KEY_NONE},
        {'i', 'I', KB_KEY_NONE, KB_KEY_NONE},
        {'o', 'O', KB_KEY_NONE, KB_KEY_NONE},
        {'p', 'P', KB_KEY_NONE, KB_KEY_NONE},
        {'[', '{', SOLAR_OS_KEY_HOME, KB_KEY_NONE},
        {']', '}', SOLAR_OS_KEY_END, KB_KEY_NONE},
        {'\\', '|', KB_KEY_NONE, KB_KEY_NONE},
    },
    {
        {0, 0, KB_KEY_NONE, KB_KEY_FN},
        {0, 0, KB_KEY_NONE, KB_KEY_SHIFT},
        {'a', 'A', KB_KEY_NONE, KB_KEY_NONE},
        {'s', 'S', KB_KEY_NONE, KB_KEY_NONE},
        {'d', 'D', KB_KEY_NONE, KB_KEY_NONE},
        {'f', 'F', KB_KEY_NONE, KB_KEY_NONE},
        {'g', 'G', KB_KEY_NONE, KB_KEY_NONE},
        {'h', 'H', KB_KEY_NONE, KB_KEY_NONE},
        {'j', 'J', KB_KEY_NONE, KB_KEY_NONE},
        {'k', 'K', KB_KEY_NONE, KB_KEY_NONE},
        {'l', 'L', KB_KEY_NONE, KB_KEY_NONE},
        {';', ':', SOLAR_OS_KEY_UP, KB_KEY_NONE},
        {'\'', '"', KB_KEY_NONE, KB_KEY_NONE},
        {'\n', '\n', KB_KEY_NONE, KB_KEY_NONE},
    },
    {
        {0, 0, KB_KEY_NONE, KB_KEY_CTRL},
        {0, 0, KB_KEY_NONE, KB_KEY_OPT},
        {0, 0, KB_KEY_NONE, KB_KEY_ALT},
        {'z', 'Z', KB_KEY_NONE, KB_KEY_NONE},
        {'x', 'X', KB_KEY_NONE, KB_KEY_NONE},
        {'c', 'C', KB_KEY_NONE, KB_KEY_NONE},
        {'v', 'V', KB_KEY_NONE, KB_KEY_NONE},
        {'b', 'B', KB_KEY_NONE, KB_KEY_NONE},
        {'n', 'N', KB_KEY_NONE, KB_KEY_NONE},
        {'m', 'M', KB_KEY_NONE, KB_KEY_NONE},
        {',', '<', SOLAR_OS_KEY_LEFT, KB_KEY_NONE},
        {'.', '>', SOLAR_OS_KEY_DOWN, KB_KEY_NONE},
        {'/', '?', SOLAR_OS_KEY_RIGHT, KB_KEY_NONE},
        {' ', ' ', KB_KEY_NONE, KB_KEY_NONE},
    },
};

static const char *TAG = "kb_cardputer";

static const gpio_num_t kb_output_pins[3] = {
    SOLAR_OS_BOARD_KEYBOARD_PIN_A0,
    SOLAR_OS_BOARD_KEYBOARD_PIN_A1,
    SOLAR_OS_BOARD_KEYBOARD_PIN_A2,
};
static const gpio_num_t kb_input_pins[SOLAR_OS_BOARD_KEYBOARD_INPUT_COUNT] =
    SOLAR_OS_BOARD_KEYBOARD_INPUT_PINS;

static QueueHandle_t char_queue;
static TaskHandle_t scan_task_handle;
static uint64_t pressed_mask;
static uint64_t debounce_mask;

typedef struct {
    bool active;
    uint8_t key_index;
    uint8_t sequence_len;
    char sequence[KB_SEQUENCE_MAX];
    uint32_t next_ms;
} kb_repeat_t;

static kb_repeat_t repeat_state;

typedef struct {
    bool fn;
    bool shift;
    bool ctrl;
    bool alt;
    bool opt;
} kb_modifiers_t;

static void queue_char(char ch)
{
    if (char_queue != NULL && xQueueSend(char_queue, &ch, 0) != pdTRUE) {
        ESP_LOGW(TAG, "keyboard char queue full");
    }
}

static uint32_t kb_now_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static uint64_t kb_scan(void)
{
    uint64_t mask = 0;

    for (int state = 0; state < 8; state++) {
        gpio_set_level(kb_output_pins[0], (state & 1) != 0);
        gpio_set_level(kb_output_pins[1], (state & 2) != 0);
        gpio_set_level(kb_output_pins[2], (state & 4) != 0);
        esp_rom_delay_us(5);

        for (int j = 0; j < SOLAR_OS_BOARD_KEYBOARD_INPUT_COUNT; j++) {
            if (gpio_get_level(kb_input_pins[j]) != 0) {
                continue; /* active low */
            }
            const int x = 2 * j + ((state > 3) ? 0 : 1);
            const int y = 3 - ((state > 3) ? (state - 4) : state);
            mask |= 1ULL << (y * KB_COLS + x);
        }
    }

    return mask;
}

static kb_modifiers_t kb_read_modifiers(uint64_t mask)
{
    kb_modifiers_t mods = {0};

    for (int y = 0; y < KB_ROWS; y++) {
        for (int x = 0; x < KB_COLS; x++) {
            if ((mask & (1ULL << (y * KB_COLS + x))) == 0) {
                continue;
            }
            switch (key_map[y][x].modifier) {
            case KB_KEY_FN: mods.fn = true; break;
            case KB_KEY_SHIFT: mods.shift = true; break;
            case KB_KEY_CTRL: mods.ctrl = true; break;
            case KB_KEY_OPT: mods.opt = true; break;
            case KB_KEY_ALT: mods.alt = true; break;
            default: break;
            }
        }
    }

    return mods;
}

static char kb_arrow_with_modifiers(uint8_t arrow, const kb_modifiers_t *mods)
{
    typedef struct {
        uint8_t base, shift, ctrl, ctrl_shift;
    } arrow_variants_t;

    const arrow_variants_t variants[] = {
        {SOLAR_OS_KEY_UP, SOLAR_OS_KEY_SHIFT_UP,
         SOLAR_OS_KEY_CTRL_UP, SOLAR_OS_KEY_CTRL_SHIFT_UP},
        {SOLAR_OS_KEY_DOWN, SOLAR_OS_KEY_SHIFT_DOWN,
         SOLAR_OS_KEY_CTRL_DOWN, SOLAR_OS_KEY_CTRL_SHIFT_DOWN},
        {SOLAR_OS_KEY_LEFT, SOLAR_OS_KEY_SHIFT_LEFT,
         SOLAR_OS_KEY_CTRL_LEFT, SOLAR_OS_KEY_CTRL_SHIFT_LEFT},
        {SOLAR_OS_KEY_RIGHT, SOLAR_OS_KEY_SHIFT_RIGHT,
         SOLAR_OS_KEY_CTRL_RIGHT, SOLAR_OS_KEY_CTRL_SHIFT_RIGHT},
    };

    for (size_t i = 0; i < sizeof(variants) / sizeof(variants[0]); i++) {
        if (variants[i].base != arrow) {
            continue;
        }
        if (mods->ctrl && mods->shift) {
            return (char)variants[i].ctrl_shift;
        }
        if (mods->ctrl) {
            return (char)variants[i].ctrl;
        }
        if (mods->shift) {
            return (char)variants[i].shift;
        }
        return (char)arrow;
    }

    return (char)arrow;
}

/* Returns emitted sequence length (0 = nothing to emit). */
static size_t kb_key_to_sequence(int y, int x, const kb_modifiers_t *mods, char *out)
{
    const kb_key_def_t *def = &key_map[y][x];
    if (def->modifier != KB_KEY_NONE) {
        return 0;
    }

    if (mods->fn && def->fn != KB_KEY_NONE) {
        out[0] = kb_arrow_with_modifiers(def->fn, mods);
        return 1;
    }

    char ch = mods->shift ? def->shifted : def->base;
    if (ch == 0) {
        return 0;
    }

    if (mods->ctrl) {
        /* Mirror the BLE keyboard's system chords. */
        if (ch == ']') {
            out[0] = (char)SOLAR_OS_KEY_APP_EXIT;
            return 1;
        }
        if (ch == '=') {
            out[0] = (char)SOLAR_OS_KEY_CTRL_PLUS;
            return 1;
        }
        if (ch == '-') {
            out[0] = (char)SOLAR_OS_KEY_CTRL_MINUS;
            return 1;
        }
        if (ch >= 'a' && ch <= 'z') {
            ch = (char)(ch - 'a' + 1);
        } else if (ch >= 'A' && ch <= 'Z') {
            ch = (char)(ch - 'A' + 1);
        }
    }

    if (mods->alt) {
        out[0] = (char)SOLAR_OS_KEY_ALT_PREFIX;
        out[1] = ch;
        return 2;
    }

    out[0] = ch;
    return 1;
}

static bool kb_repeatable(const char *sequence, size_t len)
{
    return len > 0 && (uint8_t)sequence[0] != SOLAR_OS_KEY_APP_EXIT;
}

static void kb_scan_task(void *arg)
{
    (void)arg;

    for (;;) {
        const uint64_t raw = kb_scan();
        /* Two matching scans debounce a change. */
        const uint64_t stable = raw & debounce_mask;
        const uint64_t released = pressed_mask & ~(raw | debounce_mask);
        const uint64_t newly_pressed = stable & ~pressed_mask;
        debounce_mask = raw;
        pressed_mask = (pressed_mask | newly_pressed) & ~released;

        if (repeat_state.active &&
            (pressed_mask & (1ULL << repeat_state.key_index)) == 0) {
            repeat_state.active = false;
        }

        if (newly_pressed != 0) {
            const kb_modifiers_t mods = kb_read_modifiers(pressed_mask);
            for (int index = 0; index < KB_KEY_COUNT; index++) {
                if ((newly_pressed & (1ULL << index)) == 0) {
                    continue;
                }
                char sequence[KB_SEQUENCE_MAX];
                const size_t len =
                    kb_key_to_sequence(index / KB_COLS, index % KB_COLS, &mods, sequence);
                if (len == 0) {
                    continue;
                }
                for (size_t i = 0; i < len; i++) {
                    queue_char(sequence[i]);
                }
                if (kb_repeatable(sequence, len)) {
                    repeat_state.active = true;
                    repeat_state.key_index = (uint8_t)index;
                    repeat_state.sequence_len = (uint8_t)len;
                    memcpy(repeat_state.sequence, sequence, len);
                    repeat_state.next_ms = kb_now_ms() + KB_REPEAT_DELAY_MS;
                } else {
                    repeat_state.active = false;
                }
            }
        }

        if (repeat_state.active &&
            (int32_t)(kb_now_ms() - repeat_state.next_ms) >= 0) {
            for (size_t i = 0; i < repeat_state.sequence_len; i++) {
                queue_char(repeat_state.sequence[i]);
            }
            repeat_state.next_ms = kb_now_ms() + (1000U / KB_REPEAT_RATE_CPS);
        }

        vTaskDelay(pdMS_TO_TICKS(KB_SCAN_PERIOD_MS));
    }
}

esp_err_t solar_os_keyboard_init(void)
{
    if (scan_task_handle != NULL) {
        return ESP_OK;
    }

    uint64_t output_mask = 0;
    for (size_t i = 0; i < sizeof(kb_output_pins) / sizeof(kb_output_pins[0]); i++) {
        output_mask |= 1ULL << kb_output_pins[i];
    }
    const gpio_config_t output_config = {
        .pin_bit_mask = output_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&output_config), TAG, "output gpio config failed");

    uint64_t input_mask = 0;
    for (size_t i = 0; i < SOLAR_OS_BOARD_KEYBOARD_INPUT_COUNT; i++) {
        input_mask |= 1ULL << kb_input_pins[i];
    }
    const gpio_config_t input_config = {
        .pin_bit_mask = input_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&input_config), TAG, "input gpio config failed");

    char_queue = xQueueCreate(KB_QUEUE_LEN, sizeof(char));
    if (char_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(kb_scan_task, "kb_scan", 3072, NULL, 5, &scan_task_handle) != pdPASS) {
        vQueueDelete(char_queue);
        char_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "matrix keyboard scan started");
    return ESP_OK;
}

size_t solar_os_keyboard_read_chars(char *buffer, size_t buffer_len)
{
    if (char_queue == NULL || buffer == NULL || buffer_len == 0) {
        return 0;
    }

    size_t count = 0;
    while (count < buffer_len && xQueueReceive(char_queue, &buffer[count], 0) == pdTRUE) {
        count++;
    }

    return count;
}
