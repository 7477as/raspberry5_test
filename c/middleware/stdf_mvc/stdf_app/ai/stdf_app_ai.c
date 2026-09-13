/* stdf_app_ai - AI 检测/跟踪 */

#include "stdf_app_ai.h"
#include "stdf_mvc.h"
#include "stdf_app_payload.h"
#include <stdlib.h>
#include <string.h>

#define STDF_APP_LOG(...)             do { } while (0)
#define STDF_APP_AI_ASSERT(cond)

static uint32_t           s_tick_count = 0;
static uint8_t            s_has_track  = 0;
static uint32_t           s_track_id   = 0;
static stdf_app_ai_tracking_state_t s_tracking_state = AI_TRACK_LOST;

int stdf_app_ai_init(void)
{
    return 0;
}

void stdf_app_ai_tick(void)
{
    s_tick_count++;

    if ((s_tick_count % 30) == 1) {
        uint8_t prev_has_track = s_has_track;
        stdf_app_ai_tracking_state_t prev_state = s_tracking_state;

        if (!s_has_track && (rand() % 3) == 0) {
            s_has_track = 1;
            s_track_id  = 100 + (rand() % 100);
            s_tracking_state = AI_TRACK_LOCKED;
        } else if (s_has_track && (rand() % 20) == 0) {
            s_has_track = 0;
            s_tracking_state = AI_TRACK_LOST;
        }

        if (prev_has_track != s_has_track || prev_state != s_tracking_state) {
            stdf_app_ai_tracking_t trk = {
                .state        = s_tracking_state,
                .track_id     = s_track_id,
                .object_class = AI_OBJ_PERSON,
            };
            stdf_mvc_signal_data_t p = { .ptr = &trk };
            stdf_mvc_subject_emit(STDF_MVC_SUBJECT_AI_TRACKING_STATE, &p);
        }
    }

    if (s_has_track) {
        stdf_app_ai_detection_t det = {
            .object_class = AI_OBJ_PERSON,
            .track_id     = s_track_id,
            .confidence   = 0.85f + (rand() % 15) / 100.0f,
            .bbox_x       = 800 + (rand() % 200),
            .bbox_y       = 400 + (rand() % 200),
            .bbox_w       = 200,
            .bbox_h       = 400,
            .timestamp_ms = s_tick_count * 33,
        };
        stdf_mvc_signal_data_t p = { .ptr = &det };
        stdf_mvc_subject_emit(STDF_MVC_SUBJECT_AI_DETECTION_RESULT, &p);
    }
}
