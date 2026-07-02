include("${CMAKE_CURRENT_LIST_DIR}/i2c_esp_idf.cmake")

set(SOLAR_OS_BOARD_TOUCH_DRIVER "gt911")
list(APPEND SOLAR_OS_BOARD_SRCS
    "board/solar_os_board_touch_gt911.c"
    "drivers/touch_gt911.c"
)
