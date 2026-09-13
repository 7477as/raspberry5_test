/* stdf_app_subject - 业务 subject 拼装入口 (供 stdf_mvc.h 引用) */

#ifndef __STDF_APP_SUBJECT_H__
#define __STDF_APP_SUBJECT_H__

#include "stdf_app_subject_video.h"
#include "stdf_app_subject_ai.h"
#include "stdf_app_subject_gimbal.h"
#include "stdf_app_subject_imu.h"
#include "stdf_app_subject_sensor.h"
#include "stdf_app_subject_input.h"
#include "stdf_app_subject_network.h"
#include "stdf_app_subject_storage.h"
#include "stdf_app_subject_system.h"
#include "stdf_app_subject_user.h"

#define STDF_MVC_SUBJECT_LIST \
    STDF_APP_SUBJECTS_VIDEO \
    STDF_APP_SUBJECTS_AI \
    STDF_APP_SUBJECTS_GIMBAL \
    STDF_APP_SUBJECTS_IMU \
    STDF_APP_SUBJECTS_SENSOR \
    STDF_APP_SUBJECTS_INPUT \
    STDF_APP_SUBJECTS_NETWORK \
    STDF_APP_SUBJECTS_STORAGE \
    STDF_APP_SUBJECTS_SYSTEM \
    STDF_APP_SUBJECTS_USER

#endif
