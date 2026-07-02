set(SOLAR_OS_BOARD_DISPLAY_DRIVER "mipi_dsi")
list(APPEND SOLAR_OS_BOARD_SRCS
    "board/solar_os_board_display_mipi_dsi.c"
    "drivers/lcd_mipi_dsi.c"
)
list(APPEND SOLAR_OS_BOARD_REQUIRES
    u8g2
)
