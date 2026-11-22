/* Device identity and boot uptime publisher. */
#include "stdf_app_dm_device_info.h"
#include "app/stdf_app_sampler.h"
#include "subsystem/dm/stdf_mvc_data_dm.h"
#include <string.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <unistd.h>

static stdf_mvc_dm_device_info_t g_identity;

static void read_os_version(void)
{
    FILE *file = fopen("/etc/os-release", "r");
    if (!file) return;
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "PRETTY_NAME=", 12) != 0) continue;
        char *value = line + 12;
        size_t length = strcspn(value, "\r\n");
        value[length] = '\0';
        if (length >= 2 && (value[0] == '"' || value[0] == '\'') &&
            value[length - 1] == value[0]) {
            value[length - 1] = '\0';
            value++;
        }
        snprintf(g_identity.os_version, sizeof(g_identity.os_version), "%.*s",
                 (int)sizeof(g_identity.os_version) - 1, value);
        g_identity.valid |= STDF_DM_DEVICE_OS;
        break;
    }
    fclose(file);
}

static void sample_device(void)
{
    stdf_mvc_dm_device_info_t sample = g_identity;
    sample.timestamp_ms = stdf_app_now_ms();
    struct sysinfo system_info;
    if (sysinfo(&system_info) == 0) {
        sample.uptime_seconds = (uint64_t)system_info.uptime;
        sample.valid |= STDF_DM_DEVICE_UPTIME;
    }
    stdf_app_publish(STDF_MVC_SUBJECT_DM_DEVICE_INFO, &sample);
}

static stdf_app_sampler_t g_worker = STDF_APP_SAMPLER_INIT(1000u, sample_device);

int stdf_app_dm_device_info_init(void)
{
    memset(&g_identity, 0, sizeof(g_identity));
    if (gethostname(g_identity.hostname, sizeof(g_identity.hostname)) == 0) {
        g_identity.hostname[sizeof(g_identity.hostname) - 1] = '\0';
        g_identity.valid |= STDF_DM_DEVICE_HOST;
    }
    struct utsname system_name;
    if (uname(&system_name) == 0) {
        snprintf(g_identity.kernel, sizeof(g_identity.kernel), "%s", system_name.release);
        g_identity.valid |= STDF_DM_DEVICE_KERNEL;
    }
    read_os_version();
    if (stdf_app_read_text("/sys/firmware/devicetree/base/model", g_identity.model,
                           sizeof(g_identity.model)))
        g_identity.valid |= STDF_DM_DEVICE_MODEL;
    return stdf_app_sampler_start(&g_worker);
}

void stdf_app_dm_device_info_deinit(void)
{
    stdf_app_sampler_stop(&g_worker);
}
