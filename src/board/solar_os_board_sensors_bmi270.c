#include "solar_os_board_sensors.h"

#include "bmi270.h"

esp_err_t solar_os_board_sensors_init(void)
{
    return bmi270_init();
}

esp_err_t solar_os_board_sensors_read_environment(solar_os_board_environment_t *environment)
{
    if (environment == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    bmi270_data_t data;
    const esp_err_t err = bmi270_read(&data);
    if (err != ESP_OK) {
        return err;
    }

    environment->temperature_c = data.temperature_c;
    environment->humidity_percent = 0.0f;
    return ESP_OK;
}
