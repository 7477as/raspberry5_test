/* stdf_mvc_payload_storage - 存储域 payload */

#ifndef __STDF_MVC_PAYLOAD_STORAGE_H__
#define __STDF_MVC_PAYLOAD_STORAGE_H__

#include <stdint.h>

typedef enum {
    STDF_MVC_PAYLOAD_SD_STATE_MISSING  = 0,
    STDF_MVC_PAYLOAD_SD_STATE_INSERTED = 1,
    STDF_MVC_PAYLOAD_SD_STATE_MOUNTED  = 2,
    STDF_MVC_PAYLOAD_SD_STATE_ERROR    = 3,
    STDF_MVC_PAYLOAD_SD_STATE_FULL     = 4,
} stdf_mvc_payload_sd_state_t;

typedef struct {
    stdf_mvc_payload_sd_state_t     state;
    char                    label[32];
} stdf_mvc_payload_sd_state_event_t;

typedef struct {
    uint64_t                remaining_mb;
    uint64_t                total_mb;
} stdf_mvc_payload_sd_storage_t;

typedef struct {
    char                    file_path[128];
    uint32_t                file_index;
} stdf_mvc_payload_photo_event_t;

typedef struct {
    char                    file_path[128];
    uint32_t                progress_pct;
    uint64_t                bytes_sent;
    uint64_t                bytes_total;
} stdf_mvc_payload_file_transfer_t;

typedef struct {
    uint32_t                added_count;
    uint32_t                removed_count;
} stdf_mvc_payload_file_list_t;

#endif
