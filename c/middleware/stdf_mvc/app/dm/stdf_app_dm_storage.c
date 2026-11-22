/* Root filesystem capacity and usage publisher. */
#include "stdf_app_dm_storage.h"
#include "app/stdf_app_sampler.h"
#include "subsystem/dm/stdf_mvc_data_dm.h"
#include <sys/statvfs.h>

#define STORAGE_MOUNT_POINT "/"

static void sample_storage(void)
{
    stdf_mvc_dm_storage_t sample = { .timestamp_ms = stdf_app_now_ms() };
    snprintf(sample.mount_point, sizeof(sample.mount_point), "%s", STORAGE_MOUNT_POINT);
    struct statvfs statistics;
    if (statvfs(STORAGE_MOUNT_POINT, &statistics) == 0 && statistics.f_blocks > 0 &&
        statistics.f_frsize > 0) {
        uint64_t block_size = statistics.f_frsize;
        sample.total_bytes = (uint64_t)statistics.f_blocks * block_size;
        sample.free_bytes = (uint64_t)statistics.f_bavail * block_size;
        if (sample.free_bytes <= sample.total_bytes) {
            sample.usage_percent = (float)(100.0 * (double)(sample.total_bytes - sample.free_bytes) /
                                           (double)sample.total_bytes);
            sample.valid = STDF_DM_STORAGE_TOTAL | STDF_DM_STORAGE_FREE;
        }
    }
    stdf_app_publish(STDF_MVC_SUBJECT_DM_STORAGE, &sample);
}

static stdf_app_sampler_t g_worker = STDF_APP_SAMPLER_INIT(10000u, sample_storage);

int stdf_app_dm_storage_init(void)
{
    return stdf_app_sampler_start(&g_worker);
}

void stdf_app_dm_storage_deinit(void)
{
    stdf_app_sampler_stop(&g_worker);
}
