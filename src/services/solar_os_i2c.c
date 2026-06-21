#include "solar_os_i2c.h"

#include "i2c_bus.h"

esp_err_t solar_os_i2c_init(void)
{
    return i2c_bus_init();
}

uint32_t solar_os_i2c_get_speed_hz(void)
{
    return i2c_bus_get_speed_hz();
}

int solar_os_i2c_get_sda_pin(void)
{
    return (int)i2c_bus_get_sda_pin();
}

int solar_os_i2c_get_scl_pin(void)
{
    return (int)i2c_bus_get_scl_pin();
}

esp_err_t solar_os_i2c_probe(uint8_t address)
{
    return i2c_bus_probe(address);
}

esp_err_t solar_os_i2c_read_reg(uint8_t address, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_bus_read_reg(address, reg, data, len);
}

esp_err_t solar_os_i2c_write_reg(uint8_t address, uint8_t reg, const uint8_t *data, size_t len)
{
    return i2c_bus_write_reg(address, reg, data, len);
}
