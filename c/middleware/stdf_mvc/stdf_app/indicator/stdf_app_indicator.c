/* stdf_app_indicator - LED/蜂鸣器指示器 */

#include "stdf_app_indicator.h"
#include "stdf_mvc.h"
#include "stdf_app_payload.h"
#include <stdio.h>

#define STDF_APP_LOG(...)             do { } while (0)
#define STDF_APP_INDICATOR_ASSERT(cond)

static void on_record_state(void *user_data, const stdf_mvc_signal_data_t *data);
static void on_tracking_state(void *user_data, const stdf_mvc_signal_data_t *data);
static void on_error(void *user_data, const stdf_mvc_signal_data_t *data);
static void on_button(void *user_data, const stdf_mvc_signal_data_t *data);

int stdf_app_indicator_init(void)
{
    stdf_mvc_subject_subscribe(STDF_MVC_SUBJECT_VIDEO_RECORD_STATE,
                               on_record_state, NULL);
    stdf_mvc_subject_subscribe(STDF_MVC_SUBJECT_AI_TRACKING_STATE,
                               on_tracking_state, NULL);
    stdf_mvc_subject_subscribe(STDF_MVC_SUBJECT_ERROR_REPORTED,
                               on_error, NULL);
    stdf_mvc_subject_subscribe(STDF_MVC_SUBJECT_BUTTON_PRESSED,
                               on_button, NULL);
    return 0;
}

static void on_record_state(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    stdf_app_video_record_t *rec = (stdf_app_video_record_t *)data->ptr;
    if (!rec) {
        return;
    }
    if (rec->state == VIDEO_STATE_RECORDING) {
        printf("[LED] RED ON  (recording, file=%u)\n", (unsigned)rec->file_index);
    } else if (rec->state == VIDEO_STATE_IDLE) {
        printf("[LED] RED OFF (recording stopped, dur=%ums)\n",
               (unsigned)rec->duration_ms);
    }
}

static void on_tracking_state(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    stdf_app_ai_tracking_t *trk = (stdf_app_ai_tracking_t *)data->ptr;
    if (!trk) {
        return;
    }
    static stdf_app_ai_tracking_state_t s_printed = AI_TRACK_LOST;
    if (trk->state == s_printed) {
        return;
    }
    s_printed = trk->state;
    if (trk->state == AI_TRACK_LOCKED) {
        printf("[LED] GREEN ON  (tracking id=%u)\n", (unsigned)trk->track_id);
    } else if (trk->state == AI_TRACK_LOST) {
        printf("[LED] GREEN OFF (tracking lost)\n");
    }
}

static void on_error(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    stdf_app_sys_err_t *err = (stdf_app_sys_err_t *)data->ptr;
    if (!err) {
        return;
    }
    printf("[BUZZER] BEEP! code=%d detail=%s\n", (int)err->code, err->detail);
}

static void on_button(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    stdf_app_button_event_t *btn = (stdf_app_button_event_t *)data->ptr;
    if (!btn || btn->event != BTN_EVT_PRESS) {
        return;
    }
    printf("[BTN] id=%d press\n", (int)btn->id);
}
