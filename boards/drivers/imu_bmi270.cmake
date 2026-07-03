# ponytail: no SOLAR_OS_BOARD_CAP_IMU/selector -- single consumer (the "imu"
# shell command, gated on SOLAR_OS_BOARD_M5STACK_TAB5). Promote to a full
# capability + board-abstraction service if a second IMU board shows up.
include("${CMAKE_CURRENT_LIST_DIR}/i2c_esp_idf.cmake")

list(APPEND SOLAR_OS_BOARD_SRCS
    "drivers/imu_bmi270.c"
)
