#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define RTC_PCF85063_ADDRESS 0x51

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    bool clock_integrity;
} rtc_datetime_t;

esp_err_t rtc_pcf85063_init(void);
esp_err_t rtc_pcf85063_get_datetime(rtc_datetime_t *datetime);
esp_err_t rtc_pcf85063_set_datetime(const rtc_datetime_t *datetime);
bool rtc_pcf85063_datetime_is_valid(const rtc_datetime_t *datetime);
