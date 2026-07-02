#include "rtc_rx8130ce.h"

#include "i2c_bus.h"

#define RX8130CE_CTRL1_REG    0x0E
#define RX8130CE_CTRL2_REG    0x0F
#define RX8130CE_SEC_REG      0x10
#define RX8130CE_CTRL1_24H    0x00
#define RX8130CE_CTRL1_STOP   0x02
#define RX8130CE_CTRL2_RESET  0x01

static uint8_t bcd_to_dec(uint8_t value)
{
    return (uint8_t)(((value >> 4) * 10) + (value & 0x0f));
}

static uint8_t dec_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10) << 4) | (value % 10));
}

static bool is_leap_year(uint16_t year)
{
    return ((year % 4) == 0 && (year % 100) != 0) || (year % 400) == 0;
}

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31,
    };

    if (month == 2 && is_leap_year(year)) {
        return 29;
    }

    if (month < 1 || month > 12) {
        return 0;
    }

    return days[month - 1];
}

static uint8_t weekday_for_date(uint16_t year, uint8_t month, uint8_t day)
{
    if (month < 3) {
        month += 12;
        year--;
    }

    const uint16_t k = year % 100;
    const uint16_t j = year / 100;
    const uint16_t h = (uint16_t)(day + ((13 * (month + 1)) / 5) + k + (k / 4) + (j / 4) + (5 * j)) % 7;
    return (uint8_t)((h + 6) % 7);
}

bool rtc_rx8130ce_datetime_is_valid(const rtc_datetime_t *datetime)
{
    if (datetime == NULL ||
        datetime->year < 2000 ||
        datetime->year > 2099 ||
        datetime->month < 1 ||
        datetime->month > 12 ||
        datetime->day < 1 ||
        datetime->day > days_in_month(datetime->year, datetime->month) ||
        datetime->weekday > 6 ||
        datetime->hour > 23 ||
        datetime->minute > 59 ||
        datetime->second > 59) {
        return false;
    }

    return true;
}

esp_err_t rtc_rx8130ce_init(void)
{
    uint8_t ctrl1 = 0;
    esp_err_t ret = i2c_bus_read_reg(RTC_RX8130CE_ADDRESS, RX8130CE_CTRL1_REG, &ctrl1, 1);
    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t updated = (uint8_t)(ctrl1 & ~RX8130CE_CTRL1_STOP);
    updated |= RX8130CE_CTRL1_24H;

    if (updated == ctrl1) {
        return ESP_OK;
    }

    return i2c_bus_write_reg(RTC_RX8130CE_ADDRESS, RX8130CE_CTRL1_REG, &updated, 1);
}

esp_err_t rtc_rx8130ce_get_datetime(rtc_datetime_t *datetime)
{
    if (datetime == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[7];
    const esp_err_t ret = i2c_bus_read_reg(RTC_RX8130CE_ADDRESS, RX8130CE_SEC_REG, data, sizeof(data));
    if (ret != ESP_OK) {
        return ret;
    }

    datetime->clock_integrity = true;
    datetime->second = bcd_to_dec(data[0] & 0x7f);
    datetime->minute = bcd_to_dec(data[1] & 0x7f);
    datetime->hour = bcd_to_dec(data[2] & 0x3f);
    datetime->day = bcd_to_dec(data[3] & 0x3f);
    datetime->weekday = bcd_to_dec(data[4] & 0x07);
    datetime->month = bcd_to_dec(data[5] & 0x1f);
    datetime->year = (uint16_t)(2000 + bcd_to_dec(data[6]));

    if (!rtc_rx8130ce_datetime_is_valid(datetime)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

esp_err_t rtc_rx8130ce_set_datetime(const rtc_datetime_t *datetime)
{
    if (!rtc_rx8130ce_datetime_is_valid(datetime)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[7] = {
        dec_to_bcd(datetime->second),
        dec_to_bcd(datetime->minute),
        dec_to_bcd(datetime->hour),
        dec_to_bcd(datetime->day),
        dec_to_bcd(weekday_for_date(datetime->year, datetime->month, datetime->day)),
        dec_to_bcd(datetime->month),
        dec_to_bcd((uint8_t)(datetime->year % 100)),
    };

    esp_err_t ret = rtc_rx8130ce_init();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = i2c_bus_write_reg(RTC_RX8130CE_ADDRESS, RX8130CE_SEC_REG, data, sizeof(data));
    if (ret != ESP_OK) {
        return ret;
    }

    return rtc_rx8130ce_init();
}
