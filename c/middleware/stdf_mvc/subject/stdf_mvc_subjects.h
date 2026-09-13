/* stdf_mvc_subjects - 业务 subject 拼装入口 (供 stdf_mvc.h 引用) */

#ifndef __STDF_MVC_SUBJECTS_H__
#define __STDF_MVC_SUBJECTS_H__

#include "stdf_mvc_subject_video.h"
#include "stdf_mvc_subject_ai.h"
#include "stdf_mvc_subject_gimbal.h"
#include "stdf_mvc_subject_imu.h"
#include "stdf_mvc_subject_sensor.h"
#include "stdf_mvc_subject_input.h"
#include "stdf_mvc_subject_network.h"
#include "stdf_mvc_subject_storage.h"
#include "stdf_mvc_subject_system.h"
#include "stdf_mvc_subject_user.h"

#define STDF_MVC_SUBJECT_LIST \
    STDF_MVC_SUBJECTS_VIDEO \
    STDF_MVC_SUBJECTS_AI \
    STDF_MVC_SUBJECTS_GIMBAL \
    STDF_MVC_SUBJECTS_IMU \
    STDF_MVC_SUBJECTS_SENSOR \
    STDF_MVC_SUBJECTS_INPUT \
    STDF_MVC_SUBJECTS_NETWORK \
    STDF_MVC_SUBJECTS_STORAGE \
    STDF_MVC_SUBJECTS_SYSTEM \
    STDF_MVC_SUBJECTS_USER

#endif
