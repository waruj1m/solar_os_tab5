#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "solar_os_keys.h"

#define SOLAR_OS_BLE_KEYBOARD_NAME_MAX 64
#define SOLAR_OS_BLE_KEYBOARD_SCAN_MAX_RESULTS 32
#define SOLAR_OS_BLE_KEYBOARD_MAX_REMEMBERED 1
#define SOLAR_OS_BLE_KEYBOARD_REPEAT_RATE_MIN 1U
#define SOLAR_OS_BLE_KEYBOARD_REPEAT_RATE_MAX 60U
#define SOLAR_OS_BLE_KEYBOARD_REPEAT_DELAY_MIN_MS 100U
#define SOLAR_OS_BLE_KEYBOARD_REPEAT_DELAY_MAX_MS 2000U
#define SOLAR_OS_BLE_GATT_UUID_MAX 37
#define SOLAR_OS_BLE_GATT_MAX_SERVICES 24
#define SOLAR_OS_BLE_GATT_MAX_CHARACTERISTICS 64
#define SOLAR_OS_BLE_GATT_VALUE_MAX 128

typedef enum {
    SOLAR_OS_BLE_KEYBOARD_LAYOUT_US,
    SOLAR_OS_BLE_KEYBOARD_LAYOUT_DE,
} solar_os_ble_keyboard_layout_t;

typedef struct {
    uint8_t bda[6];
    uint8_t addr_type;
    int8_t rssi;
    uint16_t appearance;
    bool hid_service;
    bool keyboard_like;
    bool remembered;
    bool connected;
    char name[SOLAR_OS_BLE_KEYBOARD_NAME_MAX];
} solar_os_ble_keyboard_scan_result_t;

typedef struct {
    bool connected;
    uint8_t bda[6];
    uint8_t addr_type;
    uint16_t conn_id;
    uint16_t mtu;
    size_t service_count;
    char status[80];
} solar_os_ble_gatt_status_t;

typedef struct {
    uint16_t start_handle;
    uint16_t end_handle;
    bool primary;
    char uuid[SOLAR_OS_BLE_GATT_UUID_MAX];
} solar_os_ble_gatt_service_t;

typedef struct {
    uint16_t handle;
    uint8_t properties;
    char uuid[SOLAR_OS_BLE_GATT_UUID_MAX];
} solar_os_ble_gatt_characteristic_t;

#include "solar_os_config.h"

#if SOLAR_OS_PACKAGE_SERVICE_BLE

esp_err_t solar_os_ble_keyboard_init(void);
esp_err_t solar_os_ble_keyboard_start_pairing(void);
esp_err_t solar_os_ble_keyboard_cancel_pairing(void);
esp_err_t solar_os_ble_keyboard_scan(solar_os_ble_keyboard_scan_result_t *results,
                                     size_t max_results,
                                     size_t *found);
esp_err_t solar_os_ble_keyboard_forget(void);
esp_err_t solar_os_ble_keyboard_prepare_sleep(uint32_t timeout_ms);
void solar_os_ble_keyboard_resume(void);
bool solar_os_ble_keyboard_is_connected(void);
bool solar_os_ble_keyboard_is_scanning(void);
bool solar_os_ble_keyboard_is_pairing(void);
size_t solar_os_ble_keyboard_remembered_count(void);
void solar_os_ble_keyboard_get_status(char *buffer, size_t buffer_len);
size_t solar_os_ble_keyboard_read_chars(char *buffer, size_t buffer_len);
void solar_os_ble_keyboard_get_repeat(uint16_t *rate_cps, uint16_t *delay_ms);
esp_err_t solar_os_ble_keyboard_set_repeat(uint16_t rate_cps, uint16_t delay_ms);
solar_os_ble_keyboard_layout_t solar_os_ble_keyboard_layout(void);
esp_err_t solar_os_ble_keyboard_set_layout(solar_os_ble_keyboard_layout_t layout);
const char *solar_os_ble_keyboard_layout_name(solar_os_ble_keyboard_layout_t layout);
bool solar_os_ble_keyboard_parse_layout(const char *name, solar_os_ble_keyboard_layout_t *layout);
const char *solar_os_ble_keyboard_addr_type_name(uint8_t addr_type);
bool solar_os_ble_keyboard_parse_addr_type(const char *name, uint8_t *addr_type);

esp_err_t solar_os_ble_gatt_connect(const uint8_t bda[6], uint8_t addr_type, uint32_t timeout_ms);
esp_err_t solar_os_ble_gatt_disconnect(void);
void solar_os_ble_gatt_get_status(solar_os_ble_gatt_status_t *status);
esp_err_t solar_os_ble_gatt_services(solar_os_ble_gatt_service_t *services,
                                     size_t max_services,
                                     size_t *count);
esp_err_t solar_os_ble_gatt_characteristics(size_t service_index,
                                            solar_os_ble_gatt_characteristic_t *characteristics,
                                            size_t max_characteristics,
                                            size_t *count);
esp_err_t solar_os_ble_gatt_read(uint16_t handle,
                                 uint8_t *value,
                                 size_t max_len,
                                 size_t *value_len,
                                 uint32_t timeout_ms);
esp_err_t solar_os_ble_gatt_write(uint16_t handle,
                                  const uint8_t *value,
                                  size_t value_len,
                                  bool with_response,
                                  uint32_t timeout_ms);

#else /* !SOLAR_OS_PACKAGE_SERVICE_BLE */

/* Inline no-op stubs for flavors without the BLE package (e.g. ESP32-P4
 * boards, which have no Bluetooth controller and no IDF bt component).
 * Callers keep compiling unchanged; everything reports not-supported. */

#include <stdio.h>

static inline esp_err_t solar_os_ble_keyboard_init(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

static inline esp_err_t solar_os_ble_keyboard_start_pairing(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

static inline esp_err_t solar_os_ble_keyboard_cancel_pairing(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

static inline esp_err_t solar_os_ble_keyboard_scan(solar_os_ble_keyboard_scan_result_t *results,
                                                   size_t max_results,
                                                   size_t *found)
{
    (void)results;
    (void)max_results;
    if (found != NULL) {
        *found = 0;
    }
    return ESP_ERR_NOT_SUPPORTED;
}

static inline esp_err_t solar_os_ble_keyboard_forget(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

static inline esp_err_t solar_os_ble_keyboard_prepare_sleep(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return ESP_OK;
}

static inline void solar_os_ble_keyboard_resume(void)
{
}

static inline bool solar_os_ble_keyboard_is_connected(void)
{
    return false;
}

static inline bool solar_os_ble_keyboard_is_scanning(void)
{
    return false;
}

static inline bool solar_os_ble_keyboard_is_pairing(void)
{
    return false;
}

static inline size_t solar_os_ble_keyboard_remembered_count(void)
{
    return 0;
}

static inline void solar_os_ble_keyboard_get_status(char *buffer, size_t buffer_len)
{
    if (buffer != NULL && buffer_len > 0) {
        snprintf(buffer, buffer_len, "not supported");
    }
}

static inline size_t solar_os_ble_keyboard_read_chars(char *buffer, size_t buffer_len)
{
    (void)buffer;
    (void)buffer_len;
    return 0;
}

static inline void solar_os_ble_keyboard_get_repeat(uint16_t *rate_cps, uint16_t *delay_ms)
{
    if (rate_cps != NULL) {
        *rate_cps = 0;
    }
    if (delay_ms != NULL) {
        *delay_ms = 0;
    }
}

static inline esp_err_t solar_os_ble_keyboard_set_repeat(uint16_t rate_cps, uint16_t delay_ms)
{
    (void)rate_cps;
    (void)delay_ms;
    return ESP_ERR_NOT_SUPPORTED;
}

static inline solar_os_ble_keyboard_layout_t solar_os_ble_keyboard_layout(void)
{
    return SOLAR_OS_BLE_KEYBOARD_LAYOUT_US;
}

static inline esp_err_t solar_os_ble_keyboard_set_layout(solar_os_ble_keyboard_layout_t layout)
{
    (void)layout;
    return ESP_ERR_NOT_SUPPORTED;
}

static inline const char *solar_os_ble_keyboard_layout_name(solar_os_ble_keyboard_layout_t layout)
{
    (void)layout;
    return "us";
}

static inline bool solar_os_ble_keyboard_parse_layout(const char *name,
                                                      solar_os_ble_keyboard_layout_t *layout)
{
    (void)name;
    (void)layout;
    return false;
}

static inline const char *solar_os_ble_keyboard_addr_type_name(uint8_t addr_type)
{
    (void)addr_type;
    return "unknown";
}

static inline bool solar_os_ble_keyboard_parse_addr_type(const char *name, uint8_t *addr_type)
{
    (void)name;
    (void)addr_type;
    return false;
}

static inline esp_err_t solar_os_ble_gatt_connect(const uint8_t bda[6],
                                                  uint8_t addr_type,
                                                  uint32_t timeout_ms)
{
    (void)bda;
    (void)addr_type;
    (void)timeout_ms;
    return ESP_ERR_NOT_SUPPORTED;
}

static inline esp_err_t solar_os_ble_gatt_disconnect(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

static inline void solar_os_ble_gatt_get_status(solar_os_ble_gatt_status_t *status)
{
    if (status != NULL) {
        *status = (solar_os_ble_gatt_status_t){0};
        snprintf(status->status, sizeof(status->status), "not supported");
    }
}

static inline esp_err_t solar_os_ble_gatt_services(solar_os_ble_gatt_service_t *services,
                                                   size_t max_services,
                                                   size_t *count)
{
    (void)services;
    (void)max_services;
    if (count != NULL) {
        *count = 0;
    }
    return ESP_ERR_NOT_SUPPORTED;
}

static inline esp_err_t solar_os_ble_gatt_characteristics(size_t service_index,
                                                          solar_os_ble_gatt_characteristic_t *characteristics,
                                                          size_t max_characteristics,
                                                          size_t *count)
{
    (void)service_index;
    (void)characteristics;
    (void)max_characteristics;
    if (count != NULL) {
        *count = 0;
    }
    return ESP_ERR_NOT_SUPPORTED;
}

static inline esp_err_t solar_os_ble_gatt_read(uint16_t handle,
                                               uint8_t *value,
                                               size_t max_len,
                                               size_t *value_len,
                                               uint32_t timeout_ms)
{
    (void)handle;
    (void)value;
    (void)max_len;
    (void)timeout_ms;
    if (value_len != NULL) {
        *value_len = 0;
    }
    return ESP_ERR_NOT_SUPPORTED;
}

static inline esp_err_t solar_os_ble_gatt_write(uint16_t handle,
                                                const uint8_t *value,
                                                size_t value_len,
                                                bool with_response,
                                                uint32_t timeout_ms)
{
    (void)handle;
    (void)value;
    (void)value_len;
    (void)with_response;
    (void)timeout_ms;
    return ESP_ERR_NOT_SUPPORTED;
}

#endif /* SOLAR_OS_PACKAGE_SERVICE_BLE */
