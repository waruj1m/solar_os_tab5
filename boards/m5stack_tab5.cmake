set(SOLAR_OS_BOARD_ID "m5stack_tab5")
set(SOLAR_OS_BOARD_NAME "M5Stack Tab5")
set(SOLAR_OS_BOARD_DEFINE "SOLAR_OS_BOARD_M5STACK_TAB5")

include("${CMAKE_CURRENT_LIST_DIR}/drivers/cdc_usb_serial_jtag.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/i2c_esp_idf.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/display_ili9881c_dsi.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/keyboard_usb_hid.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/storage_sdmmc.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/rtc_rx8130.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/battery_ina226.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/audio_es8388_es7210.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/drivers/imu_bmi270.cmake")

set(SOLAR_OS_BOARD_HAS_PSRAM ON)
set(SOLAR_OS_BOARD_PSRAM_BYTES 33554432)
set(SOLAR_OS_BOARD_HAS_CDC ON)
set(SOLAR_OS_BOARD_HAS_I2C ON)
set(SOLAR_OS_BOARD_HAS_DISPLAY ON)
set(SOLAR_OS_BOARD_HAS_GFX ON)
# USB HID keyboard on the USB-A host port (5V rail enabled by the IO
# expander bring-up in the display init path).
set(SOLAR_OS_BOARD_HAS_KEYBOARD ON)
set(SOLAR_OS_BOARD_HAS_SD ON)
set(SOLAR_OS_BOARD_HAS_RTC ON)
set(SOLAR_OS_BOARD_HAS_BATTERY ON)
set(SOLAR_OS_BOARD_HAS_AUDIO ON)
# ESP32-P4 has no radio. WiFi comes from the on-board ESP32-C6 over SDIO via
# esp_wifi_remote/esp_hosted (transport pins in sdkconfig.tab5.defaults); BLE
# stays off (tab5 flavor excludes the service, no IDF bt component on P4).
set(SOLAR_OS_BOARD_HAS_WIFI ON)
set(SOLAR_OS_BOARD_HAS_BLE OFF)
