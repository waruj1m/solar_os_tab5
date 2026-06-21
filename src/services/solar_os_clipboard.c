#include "solar_os_clipboard.h"

#include <string.h>

#include "esp_heap_caps.h"

static char *clipboard_data;
static size_t clipboard_len;

esp_err_t solar_os_clipboard_set(const char *data, size_t len)
{
    if (len > SOLAR_OS_CLIPBOARD_MAX_BYTES) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (len > 0 && data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char *next = NULL;
    if (len > 0) {
        next = heap_caps_malloc(len + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (next == NULL) {
            return ESP_ERR_NO_MEM;
        }
        memcpy(next, data, len);
        next[len] = '\0';
    }

    heap_caps_free(clipboard_data);
    clipboard_data = next;
    clipboard_len = len;
    return ESP_OK;
}

const char *solar_os_clipboard_data(size_t *len)
{
    if (len != NULL) {
        *len = clipboard_len;
    }
    return clipboard_data;
}

size_t solar_os_clipboard_size(void)
{
    return clipboard_len;
}

void solar_os_clipboard_clear(void)
{
    heap_caps_free(clipboard_data);
    clipboard_data = NULL;
    clipboard_len = 0;
}
