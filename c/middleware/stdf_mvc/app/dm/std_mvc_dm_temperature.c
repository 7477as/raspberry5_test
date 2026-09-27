/* std_mvc_dm_temperature - 树莓派 CPU 温度发布者 */

#include "std_mvc_dm_temperature.h"
#include "util/std_mvc_log.h"
#include "stdf_mvc_api.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define CPU_TEMP_PATH                "/sys/class/thermal/thermal_zone0/temp"
#define TEMPERATURE_TICK_STEP_MS     100u
#define TEMPERATURE_SAMPLE_PERIOD_MS 1000u
#define TEMPERATURE_PUBLISH_DELTA_C  1.0f

static uint64_t s_tick_ms = 0;
static bool     s_has_published = false;
static float    s_last_published_c = 0.0f;

static uint64_t now_monotonic_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u);
}

static int read_cpu_temperature_c(float *out_c)
{
    int fd = open(CPU_TEMP_PATH, O_RDONLY);
    if (fd < 0) return -1;

    char buf[16];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    (void)close(fd);
    if (n <= 0) return -1;

    buf[n] = '\0';
    long milli_c = strtol(buf, NULL, 10);
    if (milli_c <= 0) return -1;

    *out_c = (float)milli_c / 1000.0f;
    return 0;
}

static int publish_temperature_c(float value_c)
{
    std_mvc_dm_temperature_t sample = {
        .value_c      = value_c,
        .timestamp_ms = now_monotonic_ms(),
    };
    return stdf_mvc_api_publish(STD_MVC_LOCAL,
                                STDF_MVC_SUBJECT_DM_TEMPERATURE,
                                &sample,
                                STD_MVC_PUB_ASYNC);
}

static float fabsf_local(float v)
{
    return v < 0.0f ? -v : v;
}

int std_mvc_dm_temperature_init(void)
{
    s_tick_ms = 0;
    s_has_published = false;
    s_last_published_c = 0.0f;

    float value_c = 0.0f;
    if (read_cpu_temperature_c(&value_c) != 0) {
        STD_MVC_LOG_W_TAG("[DM-TEMP]", "%s", "read cpu temperature failed");
        return 0;
    }

    s_last_published_c = value_c;
    s_has_published = true;

    int rc = publish_temperature_c(value_c);
    (void)rc;
    return 0;
}

void std_mvc_dm_temperature_tick(void)
{
    s_tick_ms += TEMPERATURE_TICK_STEP_MS;
    if ((s_tick_ms % TEMPERATURE_SAMPLE_PERIOD_MS) != 0) return;

    float value_c = 0.0f;
    if (read_cpu_temperature_c(&value_c) != 0) return;

    if (s_has_published &&
        fabsf_local(value_c - s_last_published_c) < TEMPERATURE_PUBLISH_DELTA_C) {
        return;
    }

    s_last_published_c = value_c;
    s_has_published = true;

    int rc = publish_temperature_c(value_c);
    (void)rc;
}
