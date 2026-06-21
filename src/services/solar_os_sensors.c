#include "solar_os_sensors.h"

#include <stddef.h>

#include "shtc3.h"

esp_err_t solar_os_sensors_init(void)
{
    return shtc3_init();
}

esp_err_t solar_os_sensors_read_environment(solar_os_environment_t *environment)
{
    if (environment == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    shtc3_measurement_t measurement;
    const esp_err_t ret = shtc3_read_measurement(&measurement);
    if (ret != ESP_OK) {
        return ret;
    }

    environment->temperature_c = measurement.temperature_c;
    environment->humidity_percent = measurement.humidity_percent;
    return ESP_OK;
}
