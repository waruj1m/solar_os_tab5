#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"

typedef struct {
    uart_port_t port_num;
    gpio_num_t tx_pin;
    gpio_num_t rx_pin;
    uint32_t baud_rate;
    size_t rx_buffer_size;
    size_t tx_buffer_size;
} uart_port_config_t;

esp_err_t uart_port_init(const uart_port_config_t *config);
bool uart_port_is_ready(void);
esp_err_t uart_port_set_baud_rate(uint32_t baud_rate);
esp_err_t uart_port_write(const uint8_t *data, size_t len, size_t *written);
esp_err_t uart_port_read(uint8_t *data, size_t len, uint32_t timeout_ms, size_t *read_len);
esp_err_t uart_port_get_rx_buffered(size_t *buffered);
uart_port_t uart_port_get_port_num(void);
gpio_num_t uart_port_get_tx_pin(void);
gpio_num_t uart_port_get_rx_pin(void);
uint32_t uart_port_get_baud_rate(void);
