/* stdf_app_payload_storage - 存储域 payload */

#ifndef __STDF_APP_PAYLOAD_STORAGE_H__
#define __STDF_APP_PAYLOAD_STORAGE_H__

#include <stdint.h>

typedef enum {
    SD_STATE_MISSING  = 0,
    SD_STATE_INSERTED = 1,
    SD_STATE_MOUNTED  = 2,
    SD_STATE_ERROR    = 3,
    SD_STATE_FULL     = 4,
} stdf_app_sd_state_t;

typedef struct {
    stdf_app_sd_state_t     state;
    char                    label[32];
} stdf_app_sd_state_event_t;

typedef struct {
    uint64_t                remaining_mb;
    uint64_t                total_mb;
} stdf_app_sd_storage_t;

typedef struct {
    char                    file_path[128];
    uint32_t                file_index;
} stdf_app_photo_event_t;

typedef struct {
    char                    file_path[128];
    uint32_t                progress_pct;
    uint64_t                bytes_sent;
    uint64_t                bytes_total;
} stdf_app_file_transfer_t;

typedef struct {
    uint32_t                added_count;
    uint32_t                removed_count;
} stdf_app_file_list_t;

#endif
