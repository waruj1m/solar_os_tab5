#include "solar_os_board_touch.h"

#include "touch_gt911.h"

esp_err_t solar_os_board_touch_init(void)
{
    return touch_gt911_init();
}

esp_err_t solar_os_board_touch_read(solar_os_board_touch_data_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    gt911_touch_data_t gt911_data;
    const esp_err_t err = touch_gt911_read(&gt911_data);
    if (err != ESP_OK) {
        return err;
    }

    data->point_count = gt911_data.point_count;
    for (int i = 0; i < data->point_count && i < 5; i++) {
        data->points[i].touched = gt911_data.points[i].touched;
        data->points[i].x = gt911_data.points[i].x;
        data->points[i].y = gt911_data.points[i].y;
        data->points[i].gesture = gt911_data.gesture;
    }

    return ESP_OK;
}

bool solar_os_board_touch_available(void)
{
    return true;
}
