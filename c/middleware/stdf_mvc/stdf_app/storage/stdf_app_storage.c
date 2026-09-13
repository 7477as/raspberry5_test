/* stdf_app_storage - 存储管理 */

#include "stdf_app_storage.h"
#include "stdf_mvc.h"
#include "stdf_app_payload.h"
#include "stdf_app_hal_dummy.h"
#include <string.h>

#define STDF_APP_LOG(...)             do { } while (0)
#define STDF_APP_STORAGE_ASSERT(cond)

static uint32_t s_photo_seq = 0;

static void on_photo_request(void *user_data, const stdf_mvc_signal_data_t *data);

int stdf_app_storage_init(void)
{
    stdf_mvc_subject_subscribe(STDF_MVC_SUBJECT_PHOTO_CAPTURED,
                               on_photo_request, NULL);
    return 0;
}

static void on_photo_request(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    (void)data;
    s_photo_seq++;
}

void stdf_app_storage_tick(void)
{
    const stdf_app_hal_dummy_state_t *hal = stdf_app_hal_dummy_get_state();

    stdf_app_sd_state_event_t sd_state = {
        .state = hal->sd_inserted ? SD_STATE_MOUNTED : SD_STATE_MISSING,
    };
    strncpy(sd_state.label, "luna-sd", sizeof(sd_state.label) - 1);
    stdf_mvc_signal_data_t p = { .ptr = &sd_state };
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_SD_CARD_STATE, &p);

    stdf_app_sd_storage_t sd_storage = {
        .remaining_mb = hal->sd_remaining_mb,
        .total_mb     = hal->sd_remaining_mb + 4096,
    };
    p.ptr = &sd_storage;
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_SD_STORAGE_REMAINING, &p);
}
