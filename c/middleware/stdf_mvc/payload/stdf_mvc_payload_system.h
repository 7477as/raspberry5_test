/* stdf_mvc_payload_system - 系统域 payload */

#ifndef __STDF_MVC_PAYLOAD_SYSTEM_H__
#define __STDF_MVC_PAYLOAD_SYSTEM_H__

#include <stdint.h>

typedef struct {
    uint64_t                uptime_ms;
    char                    fw_version[32];
} stdf_mvc_payload_boot_event_t;

typedef struct {
    char                    reason[32];
} stdf_mvc_payload_shutdown_event_t;

typedef struct {
    uint32_t                progress_pct;
    char                    stage[32];
} stdf_mvc_payload_fw_update_t;

typedef struct {
    char                    new_version[32];
    uint64_t                size_bytes;
} stdf_mvc_payload_ota_avail_t;

typedef enum {
    STDF_MVC_PAYLOAD_SYS_ERR_NONE         = 0,
    STDF_MVC_PAYLOAD_SYS_ERR_GENERIC      = 1,
    STDF_MVC_PAYLOAD_SYS_ERR_IO           = 2,
    STDF_MVC_PAYLOAD_SYS_ERR_NETWORK      = 3,
    STDF_MVC_PAYLOAD_SYS_ERR_STORAGE_FULL = 4,
    STDF_MVC_PAYLOAD_SYS_ERR_OVER_TEMP    = 5,
    STDF_MVC_PAYLOAD_SYS_ERR_LOW_BATTERY  = 6,
    STDF_MVC_PAYLOAD_SYS_ERR_HW_FAULT     = 7,
} stdf_mvc_payload_sys_err_code_t;

typedef struct {
    stdf_mvc_payload_sys_err_code_t  code;
    char                     detail[64];
    uint64_t                 timestamp_ms;
} stdf_mvc_payload_sys_err_t;

typedef enum {
    STDF_MVC_PAYLOAD_LOG_LVL_DEBUG = 0,
    STDF_MVC_PAYLOAD_LOG_LVL_INFO  = 1,
    STDF_MVC_PAYLOAD_LOG_LVL_WARN  = 2,
    STDF_MVC_PAYLOAD_LOG_LVL_ERROR = 3,
} stdf_mvc_payload_log_lvl_t;

typedef struct {
    stdf_mvc_payload_log_lvl_t       level;
    char                     tag[16];
    char                     message[128];
} stdf_mvc_payload_log_msg_t;

#endif
