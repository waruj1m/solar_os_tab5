#include "solar_os_board_battery.h"

#include "battery_ina226.h"

esp_err_t solar_os_board_battery_init(void)
{
    return battery_ina226_init();
}

esp_err_t solar_os_board_battery_read(solar_os_board_battery_sample_t *sample)
{
    if (sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ina226_sample_t driver_sample;
    const esp_err_t err = battery_ina226_read(&driver_sample);
    if (err != ESP_OK) {
        return err;
    }

    sample->battery_mv = driver_sample.battery_mv;
    sample->calibrated = driver_sample.calibrated;
    return ESP_OK;
}
