#include "solar_os_jobs.h"

#include <string.h>

#include "jobs/solar_os_job_registry.h"

typedef struct {
    const solar_os_job_registry_entry_t *entry;
    solar_os_job_state_t state;
    esp_err_t last_error;
    uint32_t tick_count;
    uint32_t last_tick_ms;
} solar_os_job_runtime_t;

static solar_os_job_runtime_t job_runtimes[SOLAR_OS_JOBS_MAX];
static size_t job_runtime_count;
static bool jobs_initialized;

static int job_index_by_name(const char *name)
{
    if (name == NULL || name[0] == '\0') {
        return -1;
    }

    for (size_t i = 0; i < job_runtime_count; i++) {
        if (job_runtimes[i].entry != NULL &&
            job_runtimes[i].entry->name != NULL &&
            strcmp(job_runtimes[i].entry->name, name) == 0) {
            return (int)i;
        }
    }

    return -1;
}

static bool job_status_from_runtime(size_t index, solar_os_job_status_t *status)
{
    if (status == NULL || index >= job_runtime_count || job_runtimes[index].entry == NULL) {
        return false;
    }

    const solar_os_job_runtime_t *runtime = &job_runtimes[index];
    *status = (solar_os_job_status_t){
        .name = runtime->entry->name,
        .summary = runtime->entry->summary,
        .state = runtime->state,
        .last_error = runtime->last_error,
        .tick_count = runtime->tick_count,
        .last_tick_ms = runtime->last_tick_ms,
    };
    return true;
}

esp_err_t solar_os_jobs_init(void)
{
    if (jobs_initialized) {
        return ESP_OK;
    }

    memset(job_runtimes, 0, sizeof(job_runtimes));
    job_runtime_count = solar_os_job_registry_count();
    if (job_runtime_count > SOLAR_OS_JOBS_MAX) {
        job_runtime_count = SOLAR_OS_JOBS_MAX;
    }

    for (size_t i = 0; i < job_runtime_count; i++) {
        job_runtimes[i].entry = solar_os_job_registry_get(i);
        job_runtimes[i].state = SOLAR_OS_JOB_STOPPED;
        job_runtimes[i].last_error = ESP_OK;
    }

    jobs_initialized = true;
    return ESP_OK;
}

size_t solar_os_jobs_count(void)
{
    (void)solar_os_jobs_init();
    return job_runtime_count;
}

bool solar_os_jobs_get(size_t index, solar_os_job_status_t *status)
{
    (void)solar_os_jobs_init();
    return job_status_from_runtime(index, status);
}

bool solar_os_jobs_get_by_name(const char *name, solar_os_job_status_t *status)
{
    (void)solar_os_jobs_init();

    const int index = job_index_by_name(name);
    return index >= 0 && job_status_from_runtime((size_t)index, status);
}

esp_err_t solar_os_jobs_start(solar_os_context_t *ctx, const char *name, int argc, char **argv)
{
    esp_err_t ret = solar_os_jobs_init();
    if (ret != ESP_OK) {
        return ret;
    }

    const int index = job_index_by_name(name);
    if (index < 0) {
        return ESP_ERR_NOT_FOUND;
    }
    if (argc < 0 || argc > SOLAR_OS_APP_ARG_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    for (int i = 0; i < argc; i++) {
        if (argv == NULL || argv[i] == NULL || strlen(argv[i]) >= SOLAR_OS_APP_ARG_LEN) {
            return ESP_ERR_INVALID_ARG;
        }
    }

    solar_os_job_runtime_t *runtime = &job_runtimes[index];
    if (runtime->state == SOLAR_OS_JOB_RUNNING) {
        if (runtime->entry != NULL &&
            runtime->entry->job != NULL &&
            runtime->entry->job->stop != NULL) {
            runtime->entry->job->stop(ctx);
        }
        runtime->state = SOLAR_OS_JOB_STOPPED;
    }
    if (runtime->entry == NULL || runtime->entry->job == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    ret = ESP_OK;
    if (runtime->entry->job->start != NULL) {
        ret = runtime->entry->job->start(ctx, argc, argv);
    }

    runtime->last_error = ret;
    runtime->tick_count = 0;
    runtime->last_tick_ms = 0;
    runtime->state = ret == ESP_OK ? SOLAR_OS_JOB_RUNNING : SOLAR_OS_JOB_FAILED;
    return ret;
}

esp_err_t solar_os_jobs_stop(solar_os_context_t *ctx, const char *name)
{
    esp_err_t ret = solar_os_jobs_init();
    if (ret != ESP_OK) {
        return ret;
    }

    const int index = job_index_by_name(name);
    if (index < 0) {
        return ESP_ERR_NOT_FOUND;
    }

    solar_os_job_runtime_t *runtime = &job_runtimes[index];
    if (runtime->state != SOLAR_OS_JOB_RUNNING) {
        runtime->state = SOLAR_OS_JOB_STOPPED;
        return ESP_OK;
    }
    if (runtime->entry != NULL &&
        runtime->entry->job != NULL &&
        runtime->entry->job->stop != NULL) {
        runtime->entry->job->stop(ctx);
    }

    runtime->state = SOLAR_OS_JOB_STOPPED;
    runtime->last_error = ESP_OK;
    return ESP_OK;
}

esp_err_t solar_os_jobs_mark_stopped(const char *name, esp_err_t last_error)
{
    esp_err_t ret = solar_os_jobs_init();
    if (ret != ESP_OK) {
        return ret;
    }

    const int index = job_index_by_name(name);
    if (index < 0) {
        return ESP_ERR_NOT_FOUND;
    }

    solar_os_job_runtime_t *runtime = &job_runtimes[index];
    runtime->state = SOLAR_OS_JOB_STOPPED;
    runtime->last_error = last_error;
    return ESP_OK;
}

void solar_os_jobs_tick(solar_os_context_t *ctx, uint32_t now_ms)
{
    if (solar_os_jobs_init() != ESP_OK) {
        return;
    }

    const solar_os_event_t event = {
        .type = SOLAR_OS_EVENT_TICK,
        .data.tick_ms = now_ms,
    };

    for (size_t i = 0; i < job_runtime_count; i++) {
        solar_os_job_runtime_t *runtime = &job_runtimes[i];
        if (runtime->state != SOLAR_OS_JOB_RUNNING ||
            runtime->entry == NULL ||
            runtime->entry->job == NULL ||
            runtime->entry->job->event == NULL) {
            continue;
        }

        (void)runtime->entry->job->event(ctx, &event);
        runtime->tick_count++;
        runtime->last_tick_ms = now_ms;
    }
}

const char *solar_os_job_state_name(solar_os_job_state_t state)
{
    switch (state) {
    case SOLAR_OS_JOB_STOPPED:
        return "stopped";
    case SOLAR_OS_JOB_RUNNING:
        return "running";
    case SOLAR_OS_JOB_FAILED:
        return "failed";
    default:
        return "unknown";
    }
}
