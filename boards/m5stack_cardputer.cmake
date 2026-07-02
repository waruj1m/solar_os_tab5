set(SOLAR_OS_BOARD_ID "m5stack_cardputer")
set(SOLAR_OS_BOARD_NAME "M5Stack Cardputer")
set(SOLAR_OS_BOARD_DEFINE "SOLAR_OS_BOARD_M5STACK_CARDPUTER")

include("${CMAKE_CURRENT_LIST_DIR}/drivers/cdc_usb_serial_jtag.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/uart_esp_idf.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/display_st7789.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/keyboard_cardputer.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/storage_sdspi.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/gpio_esp_idf.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/adc_esp_idf.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/battery_adc.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/pwm_esp_idf.cmake")

set(SOLAR_OS_BOARD_HAS_PSRAM OFF)
set(SOLAR_OS_BOARD_HAS_DISPLAY ON)
set(SOLAR_OS_BOARD_HAS_GFX ON)
set(SOLAR_OS_BOARD_HAS_CDC ON)
set(SOLAR_OS_BOARD_HAS_UART ON)
set(SOLAR_OS_BOARD_HAS_SD ON)
set(SOLAR_OS_BOARD_HAS_BATTERY ON)
set(SOLAR_OS_BOARD_HAS_WIFI ON)
# ponytail: BLE off — Bluedroid doesn't fit in internal RAM alongside WiFi on
# this no-PSRAM board (BTU_StartUp OOM -> bluedroid shutdown assert -> boot
# loop). Built-in matrix keyboard makes the BLE keyboard redundant here.
# Revisit with NimBLE or WiFi/BT memory tuning if BLE is ever needed.
set(SOLAR_OS_BOARD_HAS_BLE OFF)
set(SOLAR_OS_BOARD_HAS_GPIO ON)
set(SOLAR_OS_BOARD_HAS_ADC ON)
set(SOLAR_OS_BOARD_HAS_PWM ON)
set(SOLAR_OS_BOARD_HAS_KEY ON)
set(SOLAR_OS_BOARD_HAS_KEYBOARD ON)
