#include "solar_os_pwm.h"

#include "driver/gpio.h"
#include "pwm_port.h"
#include "solar_os_gpio.h"

esp_err_t solar_os_pwm_init(void)
{
    return pwm_port_init();
}

size_t solar_os_pwm_pin_count(void)
{
    size_t count = 0;
    for (size_t i = 0; i < solar_os_gpio_pin_count(); i++) {
        solar_os_gpio_pin_info_t gpio_info;
        if (solar_os_gpio_get_pin_info(i, &gpio_info) && gpio_info.runtime_allowed) {
            count++;
        }
    }
    return count;
}

bool solar_os_pwm_get_pin_info(size_t index, solar_os_pwm_pin_info_t *info)
{
    if (info == NULL) {
        return false;
    }

    size_t current = 0;
    for (size_t i = 0; i < solar_os_gpio_pin_count(); i++) {
        solar_os_gpio_pin_info_t gpio_info;
        if (!solar_os_gpio_get_pin_info(i, &gpio_info) || !gpio_info.runtime_allowed) {
            continue;
        }
        if (current++ != index) {
            continue;
        }

        pwm_port_status_t status;
        (void)pwm_port_get((gpio_num_t)gpio_info.pin, &status);
        *info = (solar_os_pwm_pin_info_t) {
            .pin = gpio_info.pin,
            .runtime_allowed = true,
            .active = status.active,
            .channel = status.active ? (int)status.channel : -1,
            .freq_hz = status.freq_hz,
            .duty_percent = status.duty_percent,
        };
        return true;
    }
    return false;
}

esp_err_t solar_os_pwm_set(int pin, uint32_t freq_hz, uint8_t duty_percent)
{
    if (!solar_os_gpio_is_runtime_allowed(pin)) {
        return ESP_ERR_NOT_ALLOWED;
    }
    if (freq_hz < SOLAR_OS_PWM_FREQ_MIN_HZ ||
        freq_hz > SOLAR_OS_PWM_FREQ_MAX_HZ ||
        duty_percent > SOLAR_OS_PWM_DUTY_MAX_PERCENT) {
        return ESP_ERR_INVALID_ARG;
    }
    return pwm_port_set((gpio_num_t)pin, freq_hz, duty_percent);
}

esp_err_t solar_os_pwm_stop(int pin)
{
    if (!solar_os_gpio_is_runtime_allowed(pin)) {
        return ESP_ERR_NOT_ALLOWED;
    }
    return pwm_port_stop((gpio_num_t)pin);
}
