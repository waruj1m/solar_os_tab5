include("${CMAKE_CURRENT_LIST_DIR}/i2c_esp_idf.cmake")

set(SOLAR_OS_BOARD_BATTERY_DRIVER "ina226")
list(APPEND SOLAR_OS_BOARD_SRCS
    "board/solar_os_board_battery_ina226.c"
)
