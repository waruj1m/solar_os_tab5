set(SOLAR_OS_BOARD_DISPLAY_DRIVER "st7789")
list(APPEND SOLAR_OS_BOARD_SRCS
    "board/solar_os_board_display_st7789.c"
    "drivers/lcd_st7789.c"
)
list(APPEND SOLAR_OS_BOARD_REQUIRES
    esp_driver_gpio
    esp_driver_spi
    esp_lcd
    u8g2
)
