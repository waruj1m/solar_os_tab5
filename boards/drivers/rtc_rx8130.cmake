include("${CMAKE_CURRENT_LIST_DIR}/i2c_esp_idf.cmake")

set(SOLAR_OS_BOARD_RTC_DRIVER "rx8130")
list(APPEND SOLAR_OS_BOARD_SRCS
    "board/solar_os_board_rtc_rx8130.c"
)
