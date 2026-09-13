/* stdf_app_payload_gimbal - 云台控制域 payload */

#ifndef __STDF_APP_PAYLOAD_GIMBAL_H__
#define __STDF_APP_PAYLOAD_GIMBAL_H__

#include <stdint.h>

typedef enum {
    GIMBAL_MODE_FREE   = 0,
    GIMBAL_MODE_FOLLOW = 1,
    GIMBAL_MODE_LOCK   = 2,
    GIMBAL_MODE_PAN    = 3,
} stdf_app_gimbal_mode_t;

typedef enum {
    GIMBAL_MOTION_STOP       = 0,
    GIMBAL_MOTION_MOVING     = 1,
    GIMBAL_MOTION_CALIBRATING= 2,
    GIMBAL_MOTION_FAULT      = 3,
} stdf_app_gimbal_motion_t;

typedef struct {
    float       yaw_deg;
    float       pitch_deg;
    float       roll_deg;
    uint64_t    timestamp_ms;
} stdf_app_gimbal_pose_t;

typedef struct {
    float       target_yaw_deg;
    float       target_pitch_deg;
} stdf_app_gimbal_target_t;

typedef struct {
    stdf_app_gimbal_mode_t    mode;
} stdf_app_gimbal_mode_event_t;

typedef struct {
    stdf_app_gimbal_motion_t  motion;
} stdf_app_gimbal_motion_event_t;

typedef struct {
    float       yaw_axis_load_pct;
    float       pitch_axis_load_pct;
} stdf_app_gimbal_overload_t;

typedef struct {
    float       stick_yaw;
    float       stick_pitch;
} stdf_app_gimbal_stick_t;

#endif
