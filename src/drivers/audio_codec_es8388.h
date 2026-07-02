#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#define ES8388_I2C_ADDRESS 0x10
#define ES7210_I2C_ADDRESS 0x40

typedef struct {
    bool initialized;
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bits_per_sample;
    uint8_t volume;
    float mic_gain_db;
    int i2s_port;
    int mclk_pin;
    int bclk_pin;
    int ws_pin;
    int din_pin;
    int dout_pin;
    int pa_pin;
    const char *output_codec;
    const char *input_codec;
} audio_codec_board_status_t;

esp_err_t audio_codec_es8388_init(void);
void audio_codec_es8388_deinit(void);
esp_err_t audio_codec_es8388_set_volume(uint8_t volume);
esp_err_t audio_codec_es8388_set_mic_gain(float gain_db);
esp_err_t audio_codec_es8388_write(const void *data, size_t len);
esp_err_t audio_codec_es8388_read(void *data, size_t len);
void audio_codec_es8388_get_status(audio_codec_board_status_t *status);
