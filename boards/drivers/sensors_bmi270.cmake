include("${CMAKE_CURRENT_LIST_DIR}/i2c_esp_idf.cmake")

set(SOLAR_OS_BOARD_SENSOR_DRIVER "bmi270")
list(APPEND SOLAR_OS_BOARD_SRCS
    "board/solar_os_board_sensors_bmi270.c"
    "drivers/bmi270.c"
)
