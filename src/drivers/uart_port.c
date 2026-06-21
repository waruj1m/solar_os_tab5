#include "uart_port.h"

#include <inttypes.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "uart_port";

static uart_port_config_t active_config;
static bool ready;

static bool same_port(uart_port_t a, uart_port_t b)
{
    return a == b;
}

esp_err_t uart_port_init(const uart_port_config_t *config)
{
    if (config == NULL || config->baud_rate == 0 ||
        config->rx_buffer_size == 0 || config->tx_buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (ready && !same_port(active_config.port_num, config->port_num)) {
        ESP_RETURN_ON_ERROR(uart_driver_delete(active_config.port_num),
                            TAG,
                            "delete previous UART driver failed");
        ready = false;
    }

    uart_config_t uart_config = {
        .baud_rate = (int)config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_RETURN_ON_ERROR(uart_param_config(config->port_num, &uart_config),
                        TAG,
                        "UART parameter config failed");
    ESP_RETURN_ON_ERROR(uart_set_pin(config->port_num,
                                     config->tx_pin,
                                     config->rx_pin,
                                     UART_PIN_NO_CHANGE,
                                     UART_PIN_NO_CHANGE),
                        TAG,
                        "UART pin config failed");

    if (!ready) {
        ESP_RETURN_ON_ERROR(uart_driver_install(config->port_num,
                                                (int)config->rx_buffer_size,
                                                (int)config->tx_buffer_size,
                                                0,
                                                NULL,
                                                0),
                            TAG,
                            "UART driver install failed");
        ready = true;
    }

    active_config = *config;
    ESP_LOGI(TAG,
             "UART%d ready: TX=%d RX=%d baud=%" PRIu32,
             (int)active_config.port_num,
             (int)active_config.tx_pin,
             (int)active_config.rx_pin,
             active_config.baud_rate);
    return ESP_OK;
}

bool uart_port_is_ready(void)
{
    return ready;
}

esp_err_t uart_port_set_baud_rate(uint32_t baud_rate)
{
    if (!ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (baud_rate == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(uart_set_baudrate(active_config.port_num, (uint32_t)baud_rate),
                        TAG,
                        "UART baud config failed");
    active_config.baud_rate = baud_rate;
    return ESP_OK;
}

esp_err_t uart_port_write(const uint8_t *data, size_t len, size_t *written)
{
    if (written != NULL) {
        *written = 0;
    }
    if (!ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (data == NULL && len > 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (len == 0) {
        return ESP_OK;
    }

    const int ret = uart_write_bytes(active_config.port_num, data, len);
    if (ret < 0) {
        return ESP_FAIL;
    }
    if (written != NULL) {
        *written = (size_t)ret;
    }
    return (size_t)ret == len ? ESP_OK : ESP_FAIL;
}

esp_err_t uart_port_read(uint8_t *data, size_t len, uint32_t timeout_ms, size_t *read_len)
{
    if (read_len != NULL) {
        *read_len = 0;
    }
    if (!ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (data == NULL && len > 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (len == 0) {
        return ESP_OK;
    }

    const int ret = uart_read_bytes(active_config.port_num,
                                    data,
                                    (uint32_t)len,
                                    pdMS_TO_TICKS(timeout_ms));
    if (ret < 0) {
        return ESP_FAIL;
    }
    if (read_len != NULL) {
        *read_len = (size_t)ret;
    }
    return ESP_OK;
}

esp_err_t uart_port_get_rx_buffered(size_t *buffered)
{
    if (buffered == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *buffered = 0;
    if (!ready) {
        return ESP_ERR_INVALID_STATE;
    }

    return uart_get_buffered_data_len(active_config.port_num, buffered);
}

uart_port_t uart_port_get_port_num(void)
{
    return active_config.port_num;
}

gpio_num_t uart_port_get_tx_pin(void)
{
    return active_config.tx_pin;
}

gpio_num_t uart_port_get_rx_pin(void)
{
    return active_config.rx_pin;
}

uint32_t uart_port_get_baud_rate(void)
{
    return active_config.baud_rate;
}
