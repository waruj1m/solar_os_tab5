set(SOLAR_OS_BOARD_DISPLAY_DRIVER "ili9881c_dsi")
list(APPEND SOLAR_OS_BOARD_SRCS
    "board/solar_os_board_display_ili9881c.c"
    "drivers/lcd_ili9881c_dsi.c"
    "drivers/pi4ioe5v6408.c"
    "drivers/touch_osk.c"
)
list(APPEND SOLAR_OS_BOARD_REQUIRES
    esp_driver_gpio
    esp_driver_i2c
    esp_driver_ledc
    esp_lcd
    esp_lcd_touch_gt911
    esp_lcd_touch_st7123
    u8g2
)
