set(SOLAR_OS_BOARD_KEYBOARD_DRIVER "cardputer_matrix")
list(APPEND SOLAR_OS_BOARD_SRCS
    "drivers/keyboard_cardputer.c"
)
list(APPEND SOLAR_OS_BOARD_REQUIRES
    esp_driver_gpio
)
