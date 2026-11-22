/* CPU temperature, frequency and utilization publisher. */
#include "stdf_app_dm_cpu.h"
#include "app/stdf_app_sampler.h"
#include "subsystem/dm/stdf_mvc_data_dm.h"
#include <inttypes.h>
#include <stdlib.h>

static uint64_t g_previous_total;
static uint64_t g_previous_idle;
static bool g_has_previous;

static void read_cpu_usage(stdf_mvc_dm_cpu_t *sample)
{
    FILE *file = fopen("/proc/stat", "r");
    if (!file) { g_has_previous = false; return; }
    uint64_t counts[10] = {0};
    int fields = fscanf(file, "cpu %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                        " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                        " %" SCNu64 " %" SCNu64,
                        &counts[0], &counts[1], &counts[2], &counts[3], &counts[4],
                        &counts[5], &counts[6], &counts[7], &counts[8], &counts[9]);
    fclose(file);
    if (fields < 4) { g_has_previous = false; return; }
    uint64_t total = 0;
    for (unsigned field = 0; field < 10; field++) total += counts[field];
    uint64_t idle = counts[3] + counts[4];
    if (g_has_previous && total > g_previous_total && idle >= g_previous_idle) {
        uint64_t elapsed = total - g_previous_total;
        uint64_t idle_elapsed = idle - g_previous_idle;
        if (idle_elapsed <= elapsed) {
            sample->usage_percent = 100.0f * (float)(elapsed - idle_elapsed) / (float)elapsed;
            sample->valid |= STDF_DM_CPU_USAGE;
        }
    }
    g_previous_total = total;
    g_previous_idle = idle;
    g_has_previous = true;
}

static void read_cpu_frequencies(stdf_mvc_dm_cpu_t *sample)
{
    for (unsigned policy = 0; policy < STDF_DM_CPU_MAX_CORES; policy++) {
        char path[128];
        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpufreq/policy%u/scaling_cur_freq", policy);
        char buffer[64];
        if (!stdf_app_read_text(path, buffer, sizeof(buffer))) break;
        char *end;
        long value = strtol(buffer, &end, 10);
        if (end == buffer || value <= 0) break;
        if (policy == 0) sample->frequency_mhz = (float)value / 1000.0f;
        sample->core_frequency_mhz[sample->core_count++] = (float)value / 1000.0f;
        if (sample->core_count == STDF_DM_CPU_MAX_CORES) break;
    }
    if (sample->core_count) sample->valid |= STDF_DM_CPU_FREQ;
}

static void sample_cpu(void)
{
    stdf_mvc_dm_cpu_t sample = { .timestamp_ms = stdf_app_now_ms() };
    char buffer[128];
    if (stdf_app_read_text("/sys/class/thermal/thermal_zone0/temp", buffer, sizeof(buffer))) {
        char *end;
        long value = strtol(buffer, &end, 10);
        if (end != buffer && value >= -40000 && value <= 150000) {
            sample.temperature_c = (float)value / 1000.0f;
            sample.valid |= STDF_DM_CPU_TEMP;
        }
    }
    read_cpu_frequencies(&sample);
    read_cpu_usage(&sample);
    stdf_app_publish(STDF_MVC_SUBJECT_DM_CPU, &sample);
}

static stdf_app_sampler_t g_worker = STDF_APP_SAMPLER_INIT(1000u, sample_cpu);

int stdf_app_dm_cpu_init(void)
{
    g_has_previous = false;
    return stdf_app_sampler_start(&g_worker);
}

void stdf_app_dm_cpu_deinit(void)
{
    stdf_app_sampler_stop(&g_worker);
}
