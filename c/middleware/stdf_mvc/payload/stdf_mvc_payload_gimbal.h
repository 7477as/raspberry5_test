/* stdf_mvc_payload_gimbal - 云台控制域 payload */

#ifndef __STDF_MVC_PAYLOAD_GIMBAL_H__
#define __STDF_MVC_PAYLOAD_GIMBAL_H__

#include <stdint.h>

typedef enum {
    STDF_MVC_PAYLOAD_GIMBAL_MODE_FREE   = 0,
    STDF_MVC_PAYLOAD_GIMBAL_MODE_FOLLOW = 1,
    STDF_MVC_PAYLOAD_GIMBAL_MODE_LOCK   = 2,
    STDF_MVC_PAYLOAD_GIMBAL_MODE_PAN    = 3,
} stdf_mvc_payload_gimbal_mode_t;

typedef enum {
    STDF_MVC_PAYLOAD_GIMBAL_MOTION_STOP       = 0,
    STDF_MVC_PAYLOAD_GIMBAL_MOTION_MOVING     = 1,
    STDF_MVC_PAYLOAD_GIMBAL_MOTION_CALIBRATING= 2,
    STDF_MVC_PAYLOAD_GIMBAL_MOTION_FAULT      = 3,
} stdf_mvc_payload_gimbal_motion_t;

typedef struct {
    float       yaw_deg;
    float       pitch_deg;
    float       roll_deg;
    uint64_t    timestamp_ms;
} stdf_mvc_payload_gimbal_pose_t;

typedef struct {
    float       target_yaw_deg;
    float       target_pitch_deg;
} stdf_mvc_payload_gimbal_target_t;

typedef struct {
    stdf_mvc_payload_gimbal_mode_t    mode;
} stdf_mvc_payload_gimbal_mode_event_t;

typedef struct {
    stdf_mvc_payload_gimbal_motion_t  motion;
} stdf_mvc_payload_gimbal_motion_event_t;

typedef struct {
    float       yaw_axis_load_pct;
    float       pitch_axis_load_pct;
} stdf_mvc_payload_gimbal_overload_t;

typedef struct {
    float       stick_yaw;
    float       stick_pitch;
} stdf_mvc_payload_gimbal_stick_t;

#endif
