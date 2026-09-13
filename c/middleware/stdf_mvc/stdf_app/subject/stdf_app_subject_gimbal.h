/* stdf_app_subject_gimbal - 云台控制域 subject */

#ifndef __STDF_APP_SUBJECT_GIMBAL_H__
#define __STDF_APP_SUBJECT_GIMBAL_H__

#define STDF_APP_SUBJECTS_GIMBAL \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_GIMBAL_POSE,               "gimbal.pose") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_GIMBAL_TARGET_POSE,        "gimbal.target_pose") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_GIMBAL_MODE_CHANGED,       "gimbal.mode") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_GIMBAL_MOTION_STATE,       "gimbal.motion") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_GIMBAL_CALIBRATION_DONE,   "gimbal.calibration_done") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_GIMBAL_OVERLOAD_WARNING,   "gimbal.overload") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_GIMBAL_STICK_RESPONSE,     "gimbal.stick_response")

#endif
