set(SOLAR_OS_BOARD_ID "m5stack_tab5")
set(SOLAR_OS_BOARD_NAME "M5Stack Tab5")
set(SOLAR_OS_BOARD_DEFINE "SOLAR_OS_BOARD_M5STACK_TAB5")

include("${CMAKE_CURRENT_LIST_DIR}/drivers/cdc_usb_serial_jtag.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/i2c_esp_idf.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/display_ili9881c_dsi.cmake")

set(SOLAR_OS_BOARD_HAS_PSRAM ON)
set(SOLAR_OS_BOARD_PSRAM_BYTES 33554432)
set(SOLAR_OS_BOARD_HAS_CDC ON)
set(SOLAR_OS_BOARD_HAS_I2C ON)
set(SOLAR_OS_BOARD_HAS_DISPLAY ON)
set(SOLAR_OS_BOARD_HAS_GFX ON)
# ESP32-P4 has no radio. WiFi comes from the ESP32-C6 co-processor via
# esp-hosted (phase 4 of the bring-up); BLE stays off (tab5 flavor excludes
# the service entirely, since the P4 target has no IDF bt component).
set(SOLAR_OS_BOARD_HAS_WIFI OFF)
set(SOLAR_OS_BOARD_HAS_BLE OFF)
