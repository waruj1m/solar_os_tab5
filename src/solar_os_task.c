#include "solar_os_task.h"

bool solar_os_task_wait_done(TaskHandle_t task,
                             volatile bool *task_done,
                             uint32_t timeout_ms)
{
    if (task == NULL) {
        return true;
    }
    if (task_done != NULL && *task_done) {
        return true;
    }

    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    if (timeout_ms > 0 && timeout_ticks == 0) {
        timeout_ticks = 1;
    }

    TickType_t poll_ticks = pdMS_TO_TICKS(SOLAR_OS_TASK_STOP_POLL_MS);
    if (poll_ticks == 0) {
        poll_ticks = 1;
    }

    const TickType_t start = xTaskGetTickCount();
    while (task_done == NULL || !*task_done) {
        const TickType_t elapsed = xTaskGetTickCount() - start;
        if (timeout_ticks == 0 || elapsed >= timeout_ticks) {
            return false;
        }

        const TickType_t remaining = timeout_ticks - elapsed;
        vTaskDelay(remaining < poll_ticks ? remaining : poll_ticks);
    }

    return true;
}
