/* Linux memory capacity and availability publisher. */
#include "stdf_app_dm_mem.h"
#include "app/stdf_app_sampler.h"
#include "subsystem/dm/stdf_mvc_data_dm.h"
#include <inttypes.h>
#include <string.h>

static void sample_memory(void)
{
    stdf_mvc_dm_mem_t sample = { .timestamp_ms = stdf_app_now_ms() };
    FILE *file = fopen("/proc/meminfo", "r");
    bool has_total = false;
    bool has_available = false;
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            char key[64];
            uint64_t value;
            if (sscanf(line, "%63s %" SCNu64, key, &value) != 2) continue;
            if (strcmp(key, "MemTotal:") == 0) {
                sample.total_bytes = value * 1024u;
                has_total = true;
            } else if (strcmp(key, "MemAvailable:") == 0) {
                sample.available_bytes = value * 1024u;
                has_available = true;
            }
        }
        fclose(file);
    }
    if (has_total && has_available && sample.total_bytes > 0 &&
        sample.available_bytes <= sample.total_bytes) {
        sample.valid = 1;
        sample.usage_percent = (float)(100.0 * (double)(sample.total_bytes -
                                sample.available_bytes) / (double)sample.total_bytes);
    }
    stdf_app_publish(STDF_MVC_SUBJECT_DM_MEM, &sample);
}

static stdf_app_sampler_t g_worker = STDF_APP_SAMPLER_INIT(2000u, sample_memory);

int stdf_app_dm_mem_init(void)
{
    return stdf_app_sampler_start(&g_worker);
}

void stdf_app_dm_mem_deinit(void)
{
    stdf_app_sampler_stop(&g_worker);
}
