/* stdf_mvc_subject_user - 用户设置域 subject */

#ifndef __STDF_MVC_SUBJECT_USER_H__
#define __STDF_MVC_SUBJECT_USER_H__

#define STDF_MVC_SUBJECTS_USER \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_USER_MODE_CHANGED,         "user.mode") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_USER_SETTING_CHANGED,      "user.setting") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_USER_PROFILE_CHANGED,      "user.profile") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_LANGUAGE_CHANGED,          "user.language")

#endif
