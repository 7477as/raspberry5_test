/* stdf_app_gimbal - 云台控制 */

#include "stdf_app_gimbal.h"
#include "stdf_mvc.h"
#include "stdf_app_payload.h"
#include <math.h>

#define STDF_APP_LOG(...)             do { } while (0)
#define STDF_APP_GIMBAL_ASSERT(cond)

static float        s_current_yaw   = 0.0f;
static float        s_current_pitch = 0.0f;
static float        s_target_yaw    = 0.0f;
static float        s_target_pitch  = 0.0f;
static float        s_last_bbox_x   = 960.0f;
static float        s_last_bbox_y   = 540.0f;
static uint32_t     s_tick_count    = 0;

static void on_ai_detection(void *user_data, const stdf_mvc_signal_data_t *data);
static void on_ai_tracking(void *user_data, const stdf_mvc_signal_data_t *data);
static void on_gimbal_mode(void *user_data, const stdf_mvc_signal_data_t *data);

int stdf_app_gimbal_init(void)
{
    stdf_mvc_subject_subscribe(STDF_MVC_SUBJECT_AI_DETECTION_RESULT,
                               on_ai_detection, NULL);
    stdf_mvc_subject_subscribe(STDF_MVC_SUBJECT_AI_TRACKING_STATE,
                               on_ai_tracking, NULL);
    stdf_mvc_subject_subscribe(STDF_MVC_SUBJECT_GIMBAL_MODE_CHANGED,
                               on_gimbal_mode, NULL);
    return 0;
}

static void on_ai_detection(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    stdf_app_ai_detection_t *det = (stdf_app_ai_detection_t *)data->ptr;
    if (!det) {
        return;
    }
    float center_x = (float)det->bbox_x + (float)det->bbox_w * 0.5f;
    float center_y = (float)det->bbox_y + (float)det->bbox_h * 0.5f;

    float err_x = (center_x - 960.0f) / 1920.0f;
    float err_y = (center_y - 540.0f) / 1080.0f;

    s_target_yaw   = s_current_yaw   - err_x * 30.0f;
    s_target_pitch = s_current_pitch + err_y * 20.0f;

    s_last_bbox_x = center_x;
    s_last_bbox_y = center_y;

    stdf_app_gimbal_target_t tgt = {
        .target_yaw_deg   = s_target_yaw,
        .target_pitch_deg = s_target_pitch,
    };
    stdf_mvc_signal_data_t p = { .ptr = &tgt };
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_GIMBAL_TARGET_POSE, &p);
}

static void on_ai_tracking(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    stdf_app_ai_tracking_t *trk = (stdf_app_ai_tracking_t *)data->ptr;
    if (!trk) {
        return;
    }
    stdf_app_gimbal_motion_event_t evt = {
        .motion = (trk->state == AI_TRACK_LOCKED)
                  ? GIMBAL_MOTION_MOVING
                  : GIMBAL_MOTION_STOP,
    };
    stdf_mvc_signal_data_t p = { .ptr = &evt };
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_GIMBAL_MOTION_STATE, &p);
}

static void on_gimbal_mode(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    (void)data;
}

void stdf_app_gimbal_tick(void)
{
    s_tick_count++;

    float kp = 0.05f;
    s_current_yaw   += (s_target_yaw   - s_current_yaw)   * kp;
    s_current_pitch += (s_target_pitch - s_current_pitch) * kp;

    if (s_current_yaw >  180.0f) s_current_yaw -= 360.0f;
    if (s_current_yaw < -180.0f) s_current_yaw += 360.0f;
    if (s_current_pitch >  90.0f) s_current_pitch = 90.0f;
    if (s_current_pitch < -90.0f) s_current_pitch = -90.0f;

    stdf_app_gimbal_pose_t pose = {
        .yaw_deg     = s_current_yaw,
        .pitch_deg   = s_current_pitch,
        .roll_deg    = 0.0f,
        .timestamp_ms = s_tick_count * 33,
    };
    stdf_mvc_signal_data_t p = { .ptr = &pose };
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_GIMBAL_POSE, &p);
}
