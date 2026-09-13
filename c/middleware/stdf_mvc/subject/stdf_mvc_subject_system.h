/* stdf_mvc_subject_system - 系统域 subject */

#ifndef __STDF_MVC_SUBJECT_SYSTEM_H__
#define __STDF_MVC_SUBJECT_SYSTEM_H__

#define STDF_MVC_SUBJECTS_SYSTEM \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_SYSTEM_BOOT,               "system.boot") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_SYSTEM_SHUTDOWN,           "system.shutdown") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_FIRMWARE_UPDATE_PROGRESS,  "fw.update_progress") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_OTA_AVAILABLE,             "fw.ota_available") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_ERROR_REPORTED,            "system.error") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_LOG_MESSAGE,               "system.log")

#endif
