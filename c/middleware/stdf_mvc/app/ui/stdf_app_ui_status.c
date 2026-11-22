/* Consistent telemetry display units and validity formatting. */
#include "stdf_app_ui_status.h"
#include "subsystem/dm/stdf_mvc_data_dm.h"
#include "subsystem/net/stdf_mvc_data_net.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

const stdf_mvc_subject_id_t g_ui_subjects[STDF_UI_GROUPS] = {
    STDF_MVC_SUBJECT_DM_CPU, STDF_MVC_SUBJECT_DM_MEM, STDF_MVC_SUBJECT_DM_DEVICE_INFO,
    STDF_MVC_SUBJECT_DM_POWER, STDF_MVC_SUBJECT_NET_INTERFACES, STDF_MVC_SUBJECT_DM_STORAGE
};
const char *const g_ui_titles[STDF_UI_GROUPS] = {
    "CPU", "MEMORY", "DEVICE", "POWER", "NETWORK", "STORAGE"
};

static void append_line(stdf_app_ui_status_t *output, const char *format, ...)
{
    if (output->count >= STDF_UI_LINES) return;
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(output->lines[output->count++], STDF_UI_LINE_LENGTH, format, arguments);
    va_end(arguments);
}

static void format_cpu(const stdf_mvc_dm_cpu_t *sample, stdf_app_ui_status_t *output)
{
    char temperature[32];
    if (sample->valid & STDF_DM_CPU_TEMP)
        snprintf(temperature, sizeof(temperature), "%.1f C", (double)sample->temperature_c);
    else snprintf(temperature, sizeof(temperature), "N/A");
    char usage[32];
    if (sample->valid & STDF_DM_CPU_USAGE)
        snprintf(usage, sizeof(usage), "%.1f%%", (double)sample->usage_percent);
    else snprintf(usage, sizeof(usage), "N/A");
    char frequency[256] = "N/A";
    if (sample->valid & STDF_DM_CPU_FREQ) {
        if (sample->core_count <= 1) {
            snprintf(frequency, sizeof(frequency), "%.0f MHz", (double)sample->frequency_mhz);
        } else {
            size_t used = 0;
            frequency[0] = '\0';
            for (unsigned core = 0; core < sample->core_count; core++) {
                int written = snprintf(frequency + used, sizeof(frequency) - used, "%s%.0f",
                                       core ? "/" : "", (double)sample->core_frequency_mhz[core]);
                if (written < 0 || (size_t)written >= sizeof(frequency) - used) break;
                used += (size_t)written;
            }
            snprintf(frequency + used, sizeof(frequency) - used, " MHz");
        }
    }
    append_line(output, "TEMP: %s | USAGE: %s | FREQ: %s", temperature, usage, frequency);
    output->bar_percent = sample->valid & STDF_DM_CPU_USAGE ? sample->usage_percent : -1;
}

static void format_memory(const stdf_mvc_dm_mem_t *sample, stdf_app_ui_status_t *output)
{
    if (!sample->valid) { append_line(output, "TOTAL: N/A | AVAILABLE: N/A | USED: N/A"); return; }
    append_line(output, "TOTAL: %.0f MiB | AVAILABLE: %.0f MiB | USED: %.1f%%",
                (double)sample->total_bytes / 1048576.0, (double)sample->available_bytes / 1048576.0,
                (double)sample->usage_percent);
    output->bar_percent = sample->usage_percent;
}

static void format_device(const stdf_mvc_dm_device_info_t *sample, stdf_app_ui_status_t *output)
{
    char uptime[64];
    if (sample->valid & STDF_DM_DEVICE_UPTIME)
        snprintf(uptime, sizeof(uptime), "%" PRIu64 "d %02" PRIu64 ":%02" PRIu64 ":%02" PRIu64,
                 sample->uptime_seconds / 86400u, sample->uptime_seconds / 3600u % 24u,
                 sample->uptime_seconds / 60u % 60u, sample->uptime_seconds % 60u);
    else snprintf(uptime, sizeof(uptime), "N/A");
    append_line(output, "UPTIME: %s", uptime);
    append_line(output, "HOST: %s | KERNEL: %s | MODEL: %s",
                sample->valid & STDF_DM_DEVICE_HOST ? sample->hostname : "N/A",
                sample->valid & STDF_DM_DEVICE_KERNEL ? sample->kernel : "N/A",
                sample->valid & STDF_DM_DEVICE_MODEL ? sample->model : "N/A");
    append_line(output, "OS: %s", sample->valid & STDF_DM_DEVICE_OS ? sample->os_version : "N/A");
}

static void format_power(const stdf_mvc_dm_power_t *sample, stdf_app_ui_status_t *output)
{
    char throttled[16], undervoltage[16];
    if (sample->state_valid) {
        snprintf(throttled, sizeof(throttled), "%s", sample->throttled ? "YES" : "NO");
        snprintf(undervoltage, sizeof(undervoltage), "%s", sample->undervoltage ? "YES" : "NO");
    } else {
        snprintf(throttled, sizeof(throttled), "N/A");
        snprintf(undervoltage, sizeof(undervoltage), "N/A");
    }
    char fan[32];
    if (sample->fan_valid & STDF_DM_POWER_FAN_RPM)
        snprintf(fan, sizeof(fan), "%.0f RPM", (double)sample->fan_rpm);
    else snprintf(fan, sizeof(fan), "N/A");
    char summary[128];
    if (sample->valid) {
        double total_power = 0.0;
        unsigned powered_rails = 0;
        for (unsigned index = 0; index < sample->count && index < STDF_DM_POWER_MAX_RAILS; index++) {
            const stdf_mvc_dm_power_rail_t *rail = &sample->rails[index];
            if ((rail->valid & STDF_DM_POWER_VOLTAGE) && (rail->valid & STDF_DM_POWER_CURRENT)) {
                total_power += (double)rail->voltage_v * (double)rail->current_a;
                powered_rails++;
            }
        }
        snprintf(summary, sizeof(summary), "%.2f W FROM %u RAILS", total_power, powered_rails);
    } else {
        snprintf(summary, sizeof(summary), "N/A");
    }
    append_line(output, "THROTTLED: %s | UNDERVOLTAGE: %s | FAN: %s | PMIC: %s",
                throttled, undervoltage, fan, summary);
}

static void format_network(const stdf_mvc_net_interfaces_t *sample, stdf_app_ui_status_t *output)
{
    if (!sample->valid) { append_line(output, "NETWORK: N/A"); return; }
    if (!sample->count) { append_line(output, "NO INTERFACES"); return; }
    for (unsigned index = 0; index < sample->count && index < STDF_NET_MAX_INTERFACES; index++) {
        const stdf_mvc_net_interface_t *interface = &sample->interfaces[index];
        const char *kind = interface->type == STDF_NET_TYPE_WIRELESS ? "WIFI" :
                           interface->type == STDF_NET_TYPE_ETHERNET ? "ETH" : "OTHER";
        int link_up = (interface->valid & STDF_NET_LINK) && interface->link_up;
        if (!link_up) {
            append_line(output, "%s (%s): LINK DOWN", interface->name, kind);
            continue;
        }
        char signal[48] = "";
        if (interface->type == STDF_NET_TYPE_WIRELESS) {
            if (interface->valid & STDF_NET_WIFI_DBM)
                snprintf(signal, sizeof(signal), " | SIGNAL: %.0f DBM", (double)interface->wifi_signal_dbm);
            else snprintf(signal, sizeof(signal), " | SIGNAL: N/A");
        }
        char rates[96];
        if (interface->valid & STDF_NET_RATE)
            snprintf(rates, sizeof(rates), "RX %.0f KiB/s | TX %.0f KiB/s",
                     interface->receive_bytes_per_second / 1024.0,
                     interface->transmit_bytes_per_second / 1024.0);
        else snprintf(rates, sizeof(rates), "RX N/A | TX N/A");
        append_line(output, "%s (%s): %s | IP %s%s", interface->name, kind, rates,
                    interface->valid & STDF_NET_IPV4 ? interface->ipv4 : "N/A", signal);
    }
    if (sample->truncated) append_line(output, "MORE INTERFACES OMITTED");
}

static void format_storage(const stdf_mvc_dm_storage_t *sample, stdf_app_ui_status_t *output)
{
    if (!sample->valid) { append_line(output, "TOTAL: N/A | FREE: N/A | USED: N/A"); return; }
    append_line(output, "%s | TOTAL: %.0f GiB | FREE: %.0f GiB | USED: %.1f%%",
                sample->mount_point, (double)sample->total_bytes / 1073741824.0,
                (double)sample->free_bytes / 1073741824.0, (double)sample->usage_percent);
    output->bar_percent = sample->usage_percent;
}

void stdf_app_ui_status_format(unsigned group, const void *payload, stdf_app_ui_status_t *output)
{
    memset(output, 0, sizeof(*output));
    output->bar_percent = -1;
    if (!payload || group >= STDF_UI_GROUPS) return;
    memcpy(&output->timestamp_ms, payload, sizeof(output->timestamp_ms));
    switch (group) {
    case 0: format_cpu(payload, output); break;
    case 1: format_memory(payload, output); break;
    case 2: format_device(payload, output); break;
    case 3: format_power(payload, output); break;
    case 4: format_network(payload, output); break;
    case 5: format_storage(payload, output); break;
    default: break;
    }
}

int stdf_app_ui_status_stale(unsigned group, uint64_t timestamp_ms, uint64_t now_ms)
{
    uint64_t limit = group == 3 ? 16000u : group == 5 ? 30000u : 7000u;
    return timestamp_ms != 0 && now_ms > timestamp_ms && now_ms - timestamp_ms > limit;
}
