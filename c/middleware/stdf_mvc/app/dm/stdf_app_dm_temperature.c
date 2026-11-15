/* stdf_app_dm_temperature periodically publishes CPU temperature. */
#include "stdf_app_dm_temperature.h"
#include "util/stdf_mvc_log.h"
#include "stdf_mvc_api.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define CPU_TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"
#define TEMPERATURE_SAMPLE_PERIOD_MS 1000u

static pthread_t s_temperature_thread;
static bool s_temperature_thread_started;
static atomic_bool s_temperature_running;

static uint64_t now_monotonic_ms(void)
{
    struct timespec timestamp;
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    return (uint64_t)timestamp.tv_sec * 1000u + (uint64_t)timestamp.tv_nsec / 1000000u;
}

static int read_cpu_temperature_c(float *temperature_c)
{
    int file_descriptor = open(CPU_TEMP_PATH, O_RDONLY);
    if (file_descriptor < 0) {
        return -1;
    }

    char buffer[16];
    ssize_t bytes_read = read(file_descriptor, buffer, sizeof(buffer) - 1);
    close(file_descriptor);
    if (bytes_read <= 0) {
        return -1;
    }

    buffer[bytes_read] = '\0';
    long milli_celsius = strtol(buffer, NULL, 10);
    if (milli_celsius <= 0) {
        return -1;
    }

    *temperature_c = (float)milli_celsius / 1000.0f;
    return 0;
}

static int publish_temperature(void)
{
    float temperature_c = 0.0f;
    if (read_cpu_temperature_c(&temperature_c) != 0) {
        STDF_MVC_LOG_W_TAG("[DM-TEMP]", "%s", "read cpu temperature failed");
        return -1;
    }

    stdf_mvc_dm_temperature_t sample = {
        .value_c = temperature_c,
        .timestamp_ms = now_monotonic_ms(),
    };
    return stdf_mvc_api_publish(STDF_MVC_LOCAL,
                                STDF_MVC_SUBJECT_DM_TEMPERATURE,
                                &sample,
                                STDF_MVC_PUB_ASYNC);
}

static void *temperature_thread(void *argument)
{
    (void)argument;
    struct timespec delay = {
        .tv_sec = TEMPERATURE_SAMPLE_PERIOD_MS / 1000u,
        .tv_nsec = (TEMPERATURE_SAMPLE_PERIOD_MS % 1000u) * 1000000u,
    };

    while (atomic_load(&s_temperature_running)) {
        nanosleep(&delay, NULL);
        if (atomic_load(&s_temperature_running)) {
            publish_temperature();
        }
    }
    return NULL;
}

int stdf_app_dm_temperature_init(void)
{
    int result = publish_temperature();
    if (result != 0) {
        return result;
    }

    atomic_store(&s_temperature_running, true);
    result = pthread_create(&s_temperature_thread, NULL, temperature_thread, NULL);
    if (result != 0) {
        atomic_store(&s_temperature_running, false);
        return -result;
    }

    s_temperature_thread_started = true;
    return 0;
}

void stdf_app_dm_temperature_deinit(void)
{
    if (!s_temperature_thread_started) {
        return;
    }

    atomic_store(&s_temperature_running, false);
    pthread_join(s_temperature_thread, NULL);
    s_temperature_thread_started = false;
}
