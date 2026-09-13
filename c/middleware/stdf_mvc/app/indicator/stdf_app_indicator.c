/* stdf_app_indicator - LED/蜂鸣器指示器 */

#include "stdf_define.h"
#include "stdf_app_indicator.h"
#include "stdf_mvc.h"
#include "stdf_mvc_payloads.h"

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
    stdf_mvc_payload_video_record_t *rec = (stdf_mvc_payload_video_record_t *)data->ptr;
    if (!rec) {
        return;
    }
    if (rec->state == STDF_MVC_PAYLOAD_VIDEO_STATE_RECORDING) {
        STDF_LOG_I("[LED] RED ON  (recording, file=%u)", (unsigned)rec->file_index);
    } else if (rec->state == STDF_MVC_PAYLOAD_VIDEO_STATE_IDLE) {
        STDF_LOG_I("[LED] RED OFF (recording stopped, dur=%ums)",
                   (unsigned)rec->duration_ms);
    }
}

static void on_tracking_state(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    stdf_mvc_payload_ai_tracking_t *trk = (stdf_mvc_payload_ai_tracking_t *)data->ptr;
    if (!trk) {
        return;
    }
    static stdf_mvc_payload_ai_tracking_state_t s_printed = STDF_MVC_PAYLOAD_AI_TRACK_LOST;
    if (trk->state == s_printed) {
        return;
    }
    s_printed = trk->state;
    if (trk->state == STDF_MVC_PAYLOAD_AI_TRACK_LOCKED) {
        STDF_LOG_I("[LED] GREEN ON  (tracking id=%u)", (unsigned)trk->track_id);
    } else if (trk->state == STDF_MVC_PAYLOAD_AI_TRACK_LOST) {
        STDF_LOG_I("%s", "[LED] GREEN OFF (tracking lost)");
    }
}

static void on_error(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    stdf_mvc_payload_sys_err_t *err = (stdf_mvc_payload_sys_err_t *)data->ptr;
    if (!err) {
        return;
    }
    STDF_LOG_I("[BUZZER] BEEP! code=%d detail=%s", (int)err->code, err->detail);
}

static void on_button(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    stdf_mvc_payload_button_event_t *btn = (stdf_mvc_payload_button_event_t *)data->ptr;
    if (!btn || btn->event != STDF_MVC_PAYLOAD_BTN_EVT_PRESS) {
        return;
    }
    STDF_LOG_I("[BTN] id=%d press", (int)btn->id);
}
