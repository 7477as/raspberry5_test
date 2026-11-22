/* System telemetry payloads; validity bits distinguish missing measurements. */
#ifndef __STDF_MVC_DATA_DM_H__
#define __STDF_MVC_DATA_DM_H__
#include <stdint.h>
#define STDF_DM_CPU_TEMP 1u
#define STDF_DM_CPU_FREQ 2u
#define STDF_DM_CPU_USAGE 4u
#define STDF_DM_DEVICE_UPTIME 1u
#define STDF_DM_DEVICE_HOST 2u
#define STDF_DM_DEVICE_KERNEL 4u
#define STDF_DM_DEVICE_OS 8u
#define STDF_DM_DEVICE_MODEL 16u
#define STDF_DM_POWER_VOLTAGE 1u
#define STDF_DM_POWER_CURRENT 2u
#define STDF_DM_POWER_MAX_RAILS 24u
#define STDF_DM_POWER_THROTTLED 1u
#define STDF_DM_POWER_UNDERVOLT 2u
#define STDF_DM_POWER_FAN_RPM 4u
#define STDF_DM_STORAGE_TOTAL 1u
#define STDF_DM_STORAGE_FREE 2u

#define STDF_DM_CPU_MAX_CORES 8u
typedef struct stdf_mvc_dm_cpu_t {
    uint64_t timestamp_ms;
    uint32_t valid;
    float temperature_c;
    float frequency_mhz;
    float usage_percent;
    uint32_t core_count;
    float core_frequency_mhz[STDF_DM_CPU_MAX_CORES];
} stdf_mvc_dm_cpu_t;

typedef struct stdf_mvc_dm_mem_t {
    uint64_t timestamp_ms;
    uint64_t total_bytes;
    uint64_t available_bytes;
    float usage_percent;
    uint32_t valid;
} stdf_mvc_dm_mem_t;

typedef struct stdf_mvc_dm_device_info_t {
    uint64_t timestamp_ms;
    uint64_t uptime_seconds;
    uint32_t valid;
    char hostname[256];
    char kernel[128];
    char os_version[256];
    char model[128];
} stdf_mvc_dm_device_info_t;

typedef struct stdf_mvc_dm_power_rail_t {
    char name[32];
    float voltage_v;
    float current_a;
    uint32_t valid;
} stdf_mvc_dm_power_rail_t;

typedef struct stdf_mvc_dm_power_t {
    uint64_t timestamp_ms;
    uint32_t valid;
    uint32_t count;
    uint32_t truncated;
    uint32_t state_valid;
    uint32_t throttled;
    uint32_t undervoltage;
    uint32_t fan_valid;
    float fan_rpm;
    stdf_mvc_dm_power_rail_t rails[STDF_DM_POWER_MAX_RAILS];
} stdf_mvc_dm_power_t;

typedef struct stdf_mvc_dm_storage_t {
    uint64_t timestamp_ms;
    uint64_t total_bytes;
    uint64_t free_bytes;
    float usage_percent;
    uint32_t valid;
    char mount_point[128];
} stdf_mvc_dm_storage_t;

#define STDF_MVC_DM_DATA_LIST(X) \
    X(STDF_MVC_SUBJECT_DM_CPU, stdf_mvc_dm_cpu_t) \
    X(STDF_MVC_SUBJECT_DM_MEM, stdf_mvc_dm_mem_t) \
    X(STDF_MVC_SUBJECT_DM_DEVICE_INFO, stdf_mvc_dm_device_info_t) \
    X(STDF_MVC_SUBJECT_DM_POWER, stdf_mvc_dm_power_t) \
    X(STDF_MVC_SUBJECT_DM_STORAGE, stdf_mvc_dm_storage_t)
#endif
