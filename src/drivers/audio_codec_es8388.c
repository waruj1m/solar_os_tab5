#include "audio_codec_es8388.h"

#include "esp_log.h"
#include "i2c_bus.h"

static const char *TAG = "es8388";

static audio_codec_board_status_t driver_status = {
    .initialized = false,
    .sample_rate = 48000,
    .channels = 2,
    .bits_per_sample = 16,
    .volume = 50,
    .mic_gain_db = 35.0f,
    .i2s_port = 1,
    .mclk_pin = 30,
    .bclk_pin = 27,
    .ws_pin = 29,
    .din_pin = 28,
    .dout_pin = 26,
    .pa_pin = -1,
    .output_codec = "ES8388",
    .input_codec = "ES7210",
};

esp_err_t audio_codec_es8388_init(void)
{
    uint8_t id = 0;
    esp_err_t ret = i2c_bus_read_reg(ES8388_I2C_ADDRESS, 0x00, &id, 1);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ES8388 not found at 0x%02x", ES8388_I2C_ADDRESS);
        return ret;
    }

    ESP_LOGI(TAG, "ES8388 found, chip ID: 0x%02x", id);

    ret = i2c_bus_read_reg(ES7210_I2C_ADDRESS, 0x00, &id, 1);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ES7210 found, chip ID: 0x%02x", id);
    } else {
        ESP_LOGW(TAG, "ES7210 not found at 0x%02x", ES7210_I2C_ADDRESS);
    }

    driver_status.initialized = true;
    return ESP_OK;
}

void audio_codec_es8388_deinit(void)
{
    driver_status.initialized = false;
}

esp_err_t audio_codec_es8388_set_volume(uint8_t volume)
{
    driver_status.volume = volume;
    return ESP_OK;
}

esp_err_t audio_codec_es8388_set_mic_gain(float gain_db)
{
    driver_status.mic_gain_db = gain_db;
    return ESP_OK;
}

esp_err_t audio_codec_es8388_write(const void *data, size_t len)
{
    (void)data;
    (void)len;
    return ESP_OK;
}

esp_err_t audio_codec_es8388_read(void *data, size_t len)
{
    (void)data;
    (void)len;
    return ESP_OK;
}

void audio_codec_es8388_get_status(audio_codec_board_status_t *status)
{
    if (status != NULL) {
        *status = driver_status;
    }
}
