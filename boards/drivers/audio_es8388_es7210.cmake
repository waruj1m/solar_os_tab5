include("${CMAKE_CURRENT_LIST_DIR}/i2c_esp_idf.cmake")

set(SOLAR_OS_BOARD_AUDIO_DRIVER "es8388_es7210")
# Same codec board driver as es8311_es7210; the board header selects the
# ES8388 output codec via SOLAR_OS_BOARD_AUDIO_OUT_ES8388.
list(APPEND SOLAR_OS_BOARD_SRCS
    "board/solar_os_board_audio_es8311_es7210.c"
    "drivers/audio_codec_board.c"
)
list(APPEND SOLAR_OS_BOARD_REQUIRES
    esp_codec_dev
    esp_driver_i2s
)
