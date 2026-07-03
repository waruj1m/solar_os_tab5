#include "solar_os_board_battery.h"

#include "i2c_bus.h"
#include "solar_os_board.h"

/* TI INA226 power monitor on the internal I2C bus; the bus-voltage register
 * reads the battery rail. Implemented directly against the board interface. */

#define INA226_ADDR SOLAR_OS_BOARD_BATTERY_MONITOR_ADDR
#define INA226_BUS_VOLTAGE_REG 0x02
#define INA226_BUS_VOLTAGE_LSB_UV 1250U

esp_err_t solar_os_board_battery_init(void)
{
    esp_err_t err = i2c_bus_init();
    if (err != ESP_OK) {
        return err;
    }
    /* Power-on default configuration (continuous bus+shunt) is fine. */
    return i2c_bus_probe(INA226_ADDR);
}

esp_err_t solar_os_board_battery_read(solar_os_board_battery_sample_t *sample)
{
    if (sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t raw[2];
    const esp_err_t err = i2c_bus_read_reg(INA226_ADDR, INA226_BUS_VOLTAGE_REG,
                                           raw, sizeof(raw));
    if (err != ESP_OK) {
        return err;
    }

    const uint32_t value = ((uint32_t)raw[0] << 8) | raw[1];
    sample->battery_mv = (uint16_t)((value * INA226_BUS_VOLTAGE_LSB_UV) / 1000U);
    sample->calibrated = true;
    return ESP_OK;
}
