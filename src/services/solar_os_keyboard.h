#pragma once

#include <stddef.h>

#include "esp_err.h"

/* Built-in (board-integrated) keyboard. Implemented by the board keyboard
 * driver when SOLAR_OS_BOARD_HAS_KEYBOARD is set; callers must gate on it. */
esp_err_t solar_os_keyboard_init(void);
size_t solar_os_keyboard_read_chars(char *buffer, size_t buffer_len);
