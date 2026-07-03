#include "solar_os_board_rtc.h"

#include "esp_log.h"
#include "i2c_bus.h"
#include "solar_os_board.h"

/* Epson RX8130CE on the internal I2C bus. Small enough that the board
 * interface is implemented directly, without a separate driver layer. */

#define RX8130_ADDR SOLAR_OS_BOARD_RTC_ADDR
#define RX8130_SEC_REG 0x10 /* SEC..YEAR are 7 consecutive BCD registers */
#define RX8130_FLAG_REG 0x1D
#define RX8130_CTRL0_REG 0x1E
#define RX8130_FLAG_VLF_BIT 0x02
#define RX8130_FLAG_VBLF_BIT 0x01
#define RX8130_CTRL0_STOP_BIT 0x40

static const char *TAG = "rtc_rx8130";

static uint8_t bcd_to_dec(uint8_t value)
{
    return (uint8_t)(((value >> 4) * 10) + (value & 0x0f));
}

static uint8_t dec_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10) << 4) | (value % 10));
}

esp_err_t solar_os_board_rtc_init(void)
{
    esp_err_t err = i2c_bus_init();
    if (err != ESP_OK) {
        return err;
    }

    uint8_t flags = 0;
    err = i2c_bus_read_reg(RX8130_ADDR, RX8130_FLAG_REG, &flags, 1);
    if (err != ESP_OK) {
        return err;
    }
    if (flags & RX8130_FLAG_VLF_BIT) {
        ESP_LOGW(TAG, "voltage low flag set: RTC time is not trustworthy");
    }
    return ESP_OK;
}

esp_err_t solar_os_board_rtc_get_datetime(solar_os_board_rtc_datetime_t *datetime)
{
    if (datetime == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t regs[7];
    esp_err_t err = i2c_bus_read_reg(RX8130_ADDR, RX8130_SEC_REG, regs, sizeof(regs));
    if (err != ESP_OK) {
        return err;
    }
    uint8_t flags = 0;
    err = i2c_bus_read_reg(RX8130_ADDR, RX8130_FLAG_REG, &flags, 1);
    if (err != ESP_OK) {
        return err;
    }

    datetime->second = bcd_to_dec(regs[0] & 0x7f);
    datetime->minute = bcd_to_dec(regs[1] & 0x7f);
    datetime->hour = bcd_to_dec(regs[2] & 0x3f);
    /* Week register is one-hot: bit 0 = Sunday .. bit 6 = Saturday. */
    datetime->weekday = 0;
    for (uint8_t bit = 0; bit < 7; bit++) {
        if (regs[3] & (1U << bit)) {
            datetime->weekday = bit;
            break;
        }
    }
    datetime->day = bcd_to_dec(regs[4] & 0x3f);
    datetime->month = bcd_to_dec(regs[5] & 0x1f);
    datetime->year = (uint16_t)(2000 + bcd_to_dec(regs[6]));
    datetime->clock_integrity = (flags & RX8130_FLAG_VLF_BIT) == 0;
    return ESP_OK;
}

esp_err_t solar_os_board_rtc_set_datetime(const solar_os_board_rtc_datetime_t *datetime)
{
    if (datetime == NULL ||
        datetime->year < 2000 || datetime->year > 2099 ||
        datetime->month < 1 || datetime->month > 12 ||
        datetime->day < 1 || datetime->day > 31 ||
        datetime->weekday > 6 ||
        datetime->hour > 23 || datetime->minute > 59 || datetime->second > 59) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Stop the clock while writing, then clear the low-voltage flags. */
    uint8_t ctrl = 0;
    esp_err_t err = i2c_bus_read_reg(RX8130_ADDR, RX8130_CTRL0_REG, &ctrl, 1);
    if (err != ESP_OK) {
        return err;
    }
    uint8_t stopped = ctrl | RX8130_CTRL0_STOP_BIT;
    err = i2c_bus_write_reg(RX8130_ADDR, RX8130_CTRL0_REG, &stopped, 1);
    if (err != ESP_OK) {
        return err;
    }

    const uint8_t regs[7] = {
        dec_to_bcd(datetime->second),
        dec_to_bcd(datetime->minute),
        dec_to_bcd(datetime->hour),
        (uint8_t)(1U << datetime->weekday),
        dec_to_bcd(datetime->day),
        dec_to_bcd(datetime->month),
        dec_to_bcd((uint8_t)(datetime->year - 2000)),
    };
    err = i2c_bus_write_reg(RX8130_ADDR, RX8130_SEC_REG, regs, sizeof(regs));
    if (err == ESP_OK) {
        uint8_t flags = 0;
        if (i2c_bus_read_reg(RX8130_ADDR, RX8130_FLAG_REG, &flags, 1) == ESP_OK) {
            flags &= (uint8_t)~(RX8130_FLAG_VLF_BIT | RX8130_FLAG_VBLF_BIT);
            (void)i2c_bus_write_reg(RX8130_ADDR, RX8130_FLAG_REG, &flags, 1);
        }
    }

    ctrl &= (uint8_t)~RX8130_CTRL0_STOP_BIT;
    const esp_err_t restart_err = i2c_bus_write_reg(RX8130_ADDR, RX8130_CTRL0_REG, &ctrl, 1);
    return err != ESP_OK ? err : restart_err;
}
