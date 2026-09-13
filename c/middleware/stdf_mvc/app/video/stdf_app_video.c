/* stdf_app_video - 视频录制 */

#include "stdf_app_video.h"
#include "stdf_mvc.h"
#include "stdf_mvc_payloads.h"
#include <stdlib.h>
#include <string.h>

#define STDF_APP_LOG(...)             do { } while (0)
#define STDF_APP_VIDEO_ASSERT(cond)

static uint32_t          s_frame_seq     = 0;
static uint32_t          s_tick_count    = 0;
static uint8_t           s_is_recording  = 0;
static uint64_t          s_record_start_ms = 0;
static uint32_t          s_record_index  = 0;
static uint8_t           s_dummy_nalu[64];

static void on_button_record(void *user_data, const stdf_mvc_signal_data_t *data);

int stdf_app_video_init(void)
{
    memset(s_dummy_nalu, 0xAB, sizeof(s_dummy_nalu));
    stdf_mvc_subject_subscribe(STDF_MVC_SUBJECT_BUTTON_PRESSED,
                               on_button_record, NULL);
    return 0;
}

static void on_button_record(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    stdf_mvc_payload_button_event_t *btn = (stdf_mvc_payload_button_event_t *)data->ptr;
    if (!btn || btn->id != STDF_MVC_PAYLOAD_BTN_RECORD || btn->event != STDF_MVC_PAYLOAD_BTN_EVT_PRESS) {
        return;
    }
    s_is_recording = !s_is_recording;
    if (s_is_recording) {
        s_record_start_ms = s_tick_count * 33;
        s_record_index++;
    }
    stdf_mvc_payload_video_record_t rec = {
        .state       = s_is_recording ? STDF_MVC_PAYLOAD_VIDEO_STATE_RECORDING : STDF_MVC_PAYLOAD_VIDEO_STATE_IDLE,
        .duration_ms = s_is_recording ? 0 : (s_tick_count * 33 - s_record_start_ms),
        .file_index  = s_record_index,
    };
    stdf_mvc_signal_data_t p = { .ptr = &rec };
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_VIDEO_RECORD_STATE, &p);
}

void stdf_app_video_tick(void)
{
    s_tick_count++;

    if ((s_tick_count % 3) == 0) {
        s_frame_seq++;

        stdf_mvc_payload_video_frame_raw_t raw = {
            .virt_addr   = s_dummy_nalu,
            .size_bytes  = sizeof(s_dummy_nalu),
            .width       = 1920,
            .height      = 1080,
            .timestamp_ms = s_tick_count * 33,
        };
        stdf_mvc_signal_data_t p = { .ptr = &raw };
        stdf_mvc_subject_emit(STDF_MVC_SUBJECT_VIDEO_FRAME_READY, &p);

        stdf_mvc_payload_video_frame_encoded_t enc = {
            .data        = s_dummy_nalu,
            .size_bytes  = sizeof(s_dummy_nalu),
            .codec       = STDF_MVC_PAYLOAD_VIDEO_CODEC_H264,
            .timestamp_ms = s_tick_count * 33,
            .is_keyframe = (s_frame_seq % 30) == 0,
        };
        p.ptr = &enc;
        stdf_mvc_subject_emit(STDF_MVC_SUBJECT_VIDEO_FRAME_ENCODED, &p);
    }

    if (s_is_recording) {
        stdf_mvc_payload_video_record_t rec = {
            .state       = STDF_MVC_PAYLOAD_VIDEO_STATE_RECORDING,
            .duration_ms = s_tick_count * 33 - s_record_start_ms,
            .file_index  = s_record_index,
        };
        stdf_mvc_signal_data_t p = { .ptr = &rec };
        stdf_mvc_subject_emit(STDF_MVC_SUBJECT_VIDEO_RECORD_DURATION, &p);
    }
}
