#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "solar_os.h"

#define SOLAR_OS_JOBS_MAX 8

typedef enum {
    SOLAR_OS_JOB_STOPPED,
    SOLAR_OS_JOB_RUNNING,
    SOLAR_OS_JOB_FAILED,
} solar_os_job_state_t;

typedef struct {
    const char *name;
    const char *summary;
    solar_os_job_state_t state;
    esp_err_t last_error;
    uint32_t tick_count;
    uint32_t last_tick_ms;
} solar_os_job_status_t;

esp_err_t solar_os_jobs_init(void);
size_t solar_os_jobs_count(void);
bool solar_os_jobs_get(size_t index, solar_os_job_status_t *status);
bool solar_os_jobs_get_by_name(const char *name, solar_os_job_status_t *status);
esp_err_t solar_os_jobs_start(solar_os_context_t *ctx, const char *name, int argc, char **argv);
esp_err_t solar_os_jobs_stop(solar_os_context_t *ctx, const char *name);
esp_err_t solar_os_jobs_mark_stopped(const char *name, esp_err_t last_error);
void solar_os_jobs_tick(solar_os_context_t *ctx, uint32_t now_ms);
const char *solar_os_job_state_name(solar_os_job_state_t state);
