#include "solar_os_keyboard.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "usb/hid_host.h"
#include "usb/usb_host.h"
#include "solar_os_keys.h"

/* USB HID boot-protocol keyboard on the USB-A host port. Report decoding
 * mirrors the BLE keyboard service (services/solar_os_ble_keyboard.c) so both
 * inputs behave identically; that service is excluded from radio-less builds,
 * hence the local copy of the US key map.
 * ponytail: US layout only; port the DE table from the BLE service if a
 * layout setting is ever needed here. */

#define KB_QUEUE_LEN 64
#define KB_MAX_KEYS 6
#define KB_REPEAT_RATE_CPS 15U
#define KB_REPEAT_DELAY_MS 450U
#define KB_SEQUENCE_MAX 2

#define HID_MOD_CTRL 0x11u
#define HID_MOD_SHIFT 0x22u
#define HID_MOD_LEFT_ALT 0x04u
#define HID_MOD_ALT 0x44u

static const char *TAG = "keyboard_usb";

static QueueHandle_t char_queue;
static TaskHandle_t usb_lib_task_handle;
static bool caps_lock;
static uint8_t previous_keys[KB_MAX_KEYS];

typedef struct {
    bool active;
    uint8_t keycode;
    uint8_t sequence_len;
    char sequence[KB_SEQUENCE_MAX];
    uint32_t next_ms;
} kb_repeat_t;

static kb_repeat_t repeat_state;
static portMUX_TYPE repeat_lock = portMUX_INITIALIZER_UNLOCKED;

static uint32_t kb_now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void queue_char(char ch)
{
    if (char_queue != NULL && xQueueSend(char_queue, &ch, 0) != pdTRUE) {
        ESP_LOGW(TAG, "keyboard queue full, dropping input");
    }
}

/* --- HID usage -> SolarOS char mapping (US layout) --- */

static char shifted_digit(uint8_t keycode)
{
    static const char shifted[] = {'!', '@', '#', '$', '%', '^', '&', '*', '(', ')'};
    return shifted[keycode - 0x1e];
}

static char hid_keycode_to_char_us(uint8_t keycode, bool shift)
{
    if (keycode >= 0x04 && keycode <= 0x1d) {
        const bool upper = shift ^ caps_lock;
        return (char)((upper ? 'A' : 'a') + (keycode - 0x04));
    }

    if (keycode >= 0x1e && keycode <= 0x27) {
        if (shift) {
            return shifted_digit(keycode);
        }
        return keycode == 0x27 ? '0' : (char)('1' + (keycode - 0x1e));
    }

    switch (keycode) {
    case 0x29:
        return SOLAR_OS_KEY_ESCAPE;
    case 0x28:
        return '\n';
    case 0x2a:
        return '\b';
    case 0x2b:
        return '\t';
    case 0x2c:
        return ' ';
    case 0x2d:
        return shift ? '_' : '-';
    case 0x2e:
        return shift ? '+' : '=';
    case 0x2f:
        return shift ? '{' : '[';
    case 0x30:
        return shift ? '}' : ']';
    case 0x31:
        return shift ? '|' : '\\';
    case 0x32:
        return shift ? '~' : '#';
    case 0x33:
        return shift ? ':' : ';';
    case 0x34:
        return shift ? '"' : '\'';
    case 0x35:
        return shift ? '~' : '`';
    case 0x36:
        return shift ? '<' : ',';
    case 0x37:
        return shift ? '>' : '.';
    case 0x38:
        return shift ? '?' : '/';
    default:
        return '\0';
    }
}

static char hid_keycode_to_function_key(uint8_t keycode)
{
    if (keycode >= 0x3a && keycode <= 0x45) {
        return (char)(SOLAR_OS_KEY_F1 + (keycode - 0x3a));
    }
    return '\0';
}

static char hid_keycode_to_nav_key(uint8_t keycode, uint8_t modifiers)
{
    const bool ctrl = (modifiers & HID_MOD_CTRL) != 0;
    const bool shift = (modifiers & HID_MOD_SHIFT) != 0;

    switch (keycode) {
    case 0x4a:
        if (ctrl && shift) {
            return SOLAR_OS_KEY_CTRL_SHIFT_HOME;
        }
        if (ctrl) {
            return SOLAR_OS_KEY_CTRL_HOME;
        }
        return shift ? SOLAR_OS_KEY_SHIFT_HOME : SOLAR_OS_KEY_HOME;
    case 0x4b:
        return shift ? SOLAR_OS_KEY_SHIFT_PAGE_UP : SOLAR_OS_KEY_PAGE_UP;
    case 0x4c:
        return SOLAR_OS_KEY_DELETE;
    case 0x4d:
        if (ctrl && shift) {
            return SOLAR_OS_KEY_CTRL_SHIFT_END;
        }
        if (ctrl) {
            return SOLAR_OS_KEY_CTRL_END;
        }
        return shift ? SOLAR_OS_KEY_SHIFT_END : SOLAR_OS_KEY_END;
    case 0x4e:
        return shift ? SOLAR_OS_KEY_SHIFT_PAGE_DOWN : SOLAR_OS_KEY_PAGE_DOWN;
    case 0x4f:
        if (ctrl && shift) {
            return SOLAR_OS_KEY_CTRL_SHIFT_RIGHT;
        }
        if (ctrl) {
            return SOLAR_OS_KEY_CTRL_RIGHT;
        }
        return shift ? SOLAR_OS_KEY_SHIFT_RIGHT : SOLAR_OS_KEY_RIGHT;
    case 0x50:
        if (ctrl && shift) {
            return SOLAR_OS_KEY_CTRL_SHIFT_LEFT;
        }
        if (ctrl) {
            return SOLAR_OS_KEY_CTRL_LEFT;
        }
        return shift ? SOLAR_OS_KEY_SHIFT_LEFT : SOLAR_OS_KEY_LEFT;
    case 0x51:
        if (ctrl && shift) {
            return SOLAR_OS_KEY_CTRL_SHIFT_DOWN;
        }
        if (ctrl) {
            return SOLAR_OS_KEY_CTRL_DOWN;
        }
        return shift ? SOLAR_OS_KEY_SHIFT_DOWN : SOLAR_OS_KEY_DOWN;
    case 0x52:
        if (ctrl && shift) {
            return SOLAR_OS_KEY_CTRL_SHIFT_UP;
        }
        if (ctrl) {
            return SOLAR_OS_KEY_CTRL_UP;
        }
        return shift ? SOLAR_OS_KEY_SHIFT_UP : SOLAR_OS_KEY_UP;
    default:
        return '\0';
    }
}

static char hid_keycode_to_control_char(uint8_t keycode)
{
    if (keycode >= 0x04 && keycode <= 0x1d) {
        return (char)(keycode - 0x04 + 1);
    }

    switch (keycode) {
    case 0x23:
        return 0x1e;
    case 0x2d:
        return 0x1f;
    case 0x30:
        return 0x1d;
    case 0x31:
        return 0x1c;
    default:
        return '\0';
    }
}

static char hid_keycode_to_system_key(uint8_t keycode, uint8_t modifiers)
{
    const bool ctrl = (modifiers & HID_MOD_CTRL) != 0;
    const bool alt = (modifiers & HID_MOD_ALT) != 0;

    if (ctrl && alt && keycode == 0x4c) {
        return (char)SOLAR_OS_KEY_APP_EXIT;
    }
    if (ctrl && !alt) {
        if (keycode == 0x30) {
            return (char)SOLAR_OS_KEY_APP_EXIT;
        }
        if (keycode == 0x2e) {
            return (char)SOLAR_OS_KEY_CTRL_PLUS;
        }
    }

    return '\0';
}

static char hid_keycode_to_char(uint8_t keycode, uint8_t modifiers)
{
    const bool shift = (modifiers & HID_MOD_SHIFT) != 0;
    const char system_key = hid_keycode_to_system_key(keycode, modifiers);
    const char function_key = hid_keycode_to_function_key(keycode);
    const char nav_key = hid_keycode_to_nav_key(keycode, modifiers);

    if (system_key != '\0') {
        return system_key;
    }
    if (function_key != '\0') {
        return function_key;
    }
    if (nav_key != '\0') {
        return nav_key;
    }
    if ((modifiers & HID_MOD_CTRL) != 0) {
        const char control = hid_keycode_to_control_char(keycode);
        if (control != '\0') {
            return control;
        }
    }

    return hid_keycode_to_char_us(keycode, shift);
}

/* --- auto-repeat (boot keyboards with idle rate 0 do not repeat) --- */

static bool repeatable_char(char ch)
{
    const uint8_t code = (uint8_t)ch;
    return code == '\b' || code == ' ' ||
           (code >= 0x20 && code < 0x7f) ||
           code == SOLAR_OS_KEY_UP || code == SOLAR_OS_KEY_DOWN ||
           code == SOLAR_OS_KEY_LEFT || code == SOLAR_OS_KEY_RIGHT ||
           code == SOLAR_OS_KEY_DELETE;
}

static void repeat_clear(void)
{
    portENTER_CRITICAL(&repeat_lock);
    repeat_state.active = false;
    portEXIT_CRITICAL(&repeat_lock);
}

static void repeat_start(uint8_t keycode, const char *sequence, uint8_t len)
{
    portENTER_CRITICAL(&repeat_lock);
    repeat_state.active = true;
    repeat_state.keycode = keycode;
    repeat_state.sequence_len = len;
    memcpy(repeat_state.sequence, sequence, len);
    repeat_state.next_ms = kb_now_ms() + KB_REPEAT_DELAY_MS;
    portEXIT_CRITICAL(&repeat_lock);
}

static void repeat_stop_if_released(const uint8_t *keys)
{
    portENTER_CRITICAL(&repeat_lock);
    if (repeat_state.active) {
        bool still_down = false;
        for (size_t i = 0; i < KB_MAX_KEYS; i++) {
            if (keys[i] == repeat_state.keycode) {
                still_down = true;
                break;
            }
        }
        if (!still_down) {
            repeat_state.active = false;
        }
    }
    portEXIT_CRITICAL(&repeat_lock);
}

static void repeat_queue_if_due(void)
{
    char sequence[KB_SEQUENCE_MAX];
    uint8_t sequence_len = 0;
    bool due = false;
    const uint32_t now_ms = kb_now_ms();

    portENTER_CRITICAL(&repeat_lock);
    if (repeat_state.active && (int32_t)(now_ms - repeat_state.next_ms) >= 0) {
        sequence_len = repeat_state.sequence_len;
        memcpy(sequence, repeat_state.sequence, sequence_len);
        repeat_state.next_ms = now_ms + (1000U / KB_REPEAT_RATE_CPS);
        due = true;
    }
    portEXIT_CRITICAL(&repeat_lock);

    if (!due) {
        return;
    }

    for (uint8_t i = 0; i < sequence_len; i++) {
        queue_char(sequence[i]);
    }
}

/* --- boot report processing (same flow as the BLE keyboard service) --- */

static bool key_in_report(uint8_t key, const uint8_t *keys)
{
    for (size_t i = 0; i < KB_MAX_KEYS; i++) {
        if (keys[i] == key) {
            return true;
        }
    }
    return false;
}

static void handle_keyboard_report(const uint8_t *data, size_t length)
{
    if (data == NULL || length < 8) {
        return;
    }

    const uint8_t modifiers = data[0];
    const uint8_t *keys = &data[2];

    if (keys[0] == 0x01) { /* rollover error */
        memset(previous_keys, 0, sizeof(previous_keys));
        repeat_clear();
        return;
    }

    for (size_t i = 0; i < KB_MAX_KEYS; i++) {
        const uint8_t key = keys[i];
        if (key == 0 || key_in_report(key, previous_keys)) {
            continue;
        }

        if (key == 0x39) {
            caps_lock = !caps_lock;
            continue;
        }

        if ((modifiers & HID_MOD_ALT) != 0 && key == 0x2b) { /* Alt+Tab */
            queue_char((char)SOLAR_OS_KEY_ALT_PREFIX);
            queue_char('\t');
            repeat_clear();
            continue;
        }

        const char ch = hid_keycode_to_char(key, modifiers);
        if (ch != '\0') {
            char sequence[KB_SEQUENCE_MAX];
            uint8_t sequence_len = 0;

            const bool left_alt = (modifiers & HID_MOD_LEFT_ALT) != 0;
            if (left_alt && (uint8_t)ch != SOLAR_OS_KEY_APP_EXIT) {
                queue_char((char)SOLAR_OS_KEY_ALT_PREFIX);
                sequence[sequence_len++] = (char)SOLAR_OS_KEY_ALT_PREFIX;
            }
            queue_char(ch);
            sequence[sequence_len++] = ch;

            if (repeatable_char(ch)) {
                repeat_start(key, sequence, sequence_len);
            } else {
                repeat_clear();
            }
        }
    }

    repeat_stop_if_released(keys);
    memcpy(previous_keys, keys, sizeof(previous_keys));
}

/* --- usb_host / hid_host plumbing --- */

static void hid_interface_event_cb(hid_host_device_handle_t hid_device_handle,
                                   const hid_host_interface_event_t event,
                                   void *arg)
{
    (void)arg;

    switch (event) {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT: {
        uint8_t report[16];
        size_t report_len = 0;
        if (hid_host_device_get_raw_input_report_data(hid_device_handle, report,
                                                      sizeof(report), &report_len) == ESP_OK) {
            handle_keyboard_report(report, report_len);
        }
        break;
    }

    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "keyboard disconnected");
        memset(previous_keys, 0, sizeof(previous_keys));
        repeat_clear();
        hid_host_device_close(hid_device_handle);
        break;

    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
    default:
        break;
    }
}

static void hid_driver_event_cb(hid_host_device_handle_t hid_device_handle,
                                const hid_host_driver_event_t event,
                                void *arg)
{
    (void)arg;

    if (event != HID_HOST_DRIVER_EVENT_CONNECTED) {
        return;
    }

    hid_host_dev_params_t params;
    if (hid_host_device_get_params(hid_device_handle, &params) != ESP_OK) {
        return;
    }
    if (params.proto != HID_PROTOCOL_KEYBOARD) {
        return;
    }

    const hid_host_device_config_t config = {
        .callback = hid_interface_event_cb,
        .callback_arg = NULL,
    };
    if (hid_host_device_open(hid_device_handle, &config) != ESP_OK) {
        ESP_LOGW(TAG, "keyboard open failed");
        return;
    }
    hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT);
    hid_class_request_set_idle(hid_device_handle, 0, 0);
    if (hid_host_device_start(hid_device_handle) != ESP_OK) {
        ESP_LOGW(TAG, "keyboard start failed");
        hid_host_device_close(hid_device_handle);
        return;
    }
    ESP_LOGI(TAG, "USB keyboard connected (addr %u iface %u)", params.addr, params.iface_num);
}

static void usb_lib_task(void *arg)
{
    (void)arg;

    while (true) {
        uint32_t event_flags = 0;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
    }
}

esp_err_t solar_os_keyboard_init(void)
{
    if (usb_lib_task_handle != NULL) {
        return ESP_OK;
    }

    char_queue = xQueueCreate(KB_QUEUE_LEN, sizeof(char));
    if (char_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    ESP_RETURN_ON_ERROR(usb_host_install(&host_config), TAG, "usb host install failed");

    if (xTaskCreate(usb_lib_task, "usb_events", 4096, NULL, 5, &usb_lib_task_handle) != pdPASS) {
        usb_host_uninstall();
        vQueueDelete(char_queue);
        char_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    const hid_host_driver_config_t hid_config = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = tskNO_AFFINITY,
        .callback = hid_driver_event_cb,
        .callback_arg = NULL,
    };
    const esp_err_t err = hid_host_install(&hid_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "hid host install failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "USB HID keyboard host started");
    return ESP_OK;
}

esp_err_t solar_os_keyboard_inject(const char *chars, size_t len)
{
    if (chars == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (char_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    for (size_t i = 0; i < len; i++) {
        queue_char(chars[i]);
    }
    return ESP_OK;
}

size_t solar_os_keyboard_read_chars(char *buffer, size_t buffer_len)
{
    if (char_queue == NULL || buffer == NULL || buffer_len == 0) {
        return 0;
    }

    repeat_queue_if_due();

    size_t count = 0;
    while (count < buffer_len && xQueueReceive(char_queue, &buffer[count], 0) == pdTRUE) {
        count++;
    }

    return count;
}
