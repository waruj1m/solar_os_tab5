#include "pi4ioe5v6408.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "i2c_bus.h"
#include "solar_os_board.h"

#define PI4IO_REG_CHIP_RESET 0x01
#define PI4IO_REG_IO_DIR 0x03
#define PI4IO_REG_OUT_SET 0x05
#define PI4IO_REG_OUT_H_IM 0x07
#define PI4IO_REG_IN_DEF_STA 0x09
#define PI4IO_REG_PULL_EN 0x0B
#define PI4IO_REG_PULL_SEL 0x0D
#define PI4IO_REG_INT_MASK 0x11

static const char *TAG = "pi4ioe5v6408";
static SemaphoreHandle_t out_mutex;

static esp_err_t pi4io_write(uint8_t addr, uint8_t reg, uint8_t value)
{
    return i2c_bus_write_reg(addr, reg, &value, 1);
}

esp_err_t pi4ioe5v6408_board_init(void)
{
    if (out_mutex == NULL) {
        out_mutex = xSemaphoreCreateMutex();
        if (out_mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    /* Register values are the Tab5 power-up configuration from
     * m5stack/M5Tab5-UserDemo bsp_io_expander_pi4ioe_init(). */
    const uint8_t addr1 = SOLAR_OS_BOARD_IO_EXPANDER1_ADDR;
    ESP_RETURN_ON_ERROR(pi4io_write(addr1, PI4IO_REG_CHIP_RESET, 0xFF), TAG, "exp1 reset");
    ESP_RETURN_ON_ERROR(pi4io_write(addr1, PI4IO_REG_IO_DIR, 0x7F), TAG, "exp1 dir");
    ESP_RETURN_ON_ERROR(pi4io_write(addr1, PI4IO_REG_OUT_H_IM, 0x00), TAG, "exp1 hi-z");
    ESP_RETURN_ON_ERROR(pi4io_write(addr1, PI4IO_REG_PULL_SEL, 0x7F), TAG, "exp1 pull sel");
    ESP_RETURN_ON_ERROR(pi4io_write(addr1, PI4IO_REG_PULL_EN, 0x7F), TAG, "exp1 pull en");
    /* P1 SPK_EN, P2 EXT5V_EN, P4 LCD_RST, P5 TP_RST, P6 CAM_RST high. */
    ESP_RETURN_ON_ERROR(pi4io_write(addr1, PI4IO_REG_OUT_SET, 0x76), TAG, "exp1 out");

    const uint8_t addr2 = SOLAR_OS_BOARD_IO_EXPANDER2_ADDR;
    ESP_RETURN_ON_ERROR(pi4io_write(addr2, PI4IO_REG_CHIP_RESET, 0xFF), TAG, "exp2 reset");
    ESP_RETURN_ON_ERROR(pi4io_write(addr2, PI4IO_REG_IO_DIR, 0xB9), TAG, "exp2 dir");
    ESP_RETURN_ON_ERROR(pi4io_write(addr2, PI4IO_REG_OUT_H_IM, 0x06), TAG, "exp2 hi-z");
    ESP_RETURN_ON_ERROR(pi4io_write(addr2, PI4IO_REG_PULL_SEL, 0xB9), TAG, "exp2 pull sel");
    ESP_RETURN_ON_ERROR(pi4io_write(addr2, PI4IO_REG_PULL_EN, 0xF9), TAG, "exp2 pull en");
    ESP_RETURN_ON_ERROR(pi4io_write(addr2, PI4IO_REG_IN_DEF_STA, 0x40), TAG, "exp2 in def");
    ESP_RETURN_ON_ERROR(pi4io_write(addr2, PI4IO_REG_INT_MASK, 0xBF), TAG, "exp2 int mask");
    /* P0 WLAN_PWR_EN, P3 USB5V_EN high; charge control off. */
    ESP_RETURN_ON_ERROR(pi4io_write(addr2, PI4IO_REG_OUT_SET, 0x09), TAG, "exp2 out");

    ESP_LOGI(TAG, "IO expanders configured (0x%02X, 0x%02X)", addr1, addr2);
    return ESP_OK;
}

esp_err_t pi4ioe5v6408_set_output(uint8_t i2c_addr, uint8_t pin, bool level)
{
    if (pin > 7) {
        return ESP_ERR_INVALID_ARG;
    }
    if (out_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(out_mutex, portMAX_DELAY);
    uint8_t value = 0;
    esp_err_t err = i2c_bus_read_reg(i2c_addr, PI4IO_REG_OUT_SET, &value, 1);
    if (err == ESP_OK) {
        if (level) {
            value |= (uint8_t)(1U << pin);
        } else {
            value &= (uint8_t)~(1U << pin);
        }
        err = pi4io_write(i2c_addr, PI4IO_REG_OUT_SET, value);
    }
    xSemaphoreGive(out_mutex);
    return err;
}
