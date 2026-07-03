set(SOLAR_OS_BOARD_KEYBOARD_DRIVER "usb_hid")
list(APPEND SOLAR_OS_BOARD_SRCS
    "drivers/keyboard_usb_hid.c"
)
list(APPEND SOLAR_OS_BOARD_REQUIRES
    usb
    usb_host_hid
)
