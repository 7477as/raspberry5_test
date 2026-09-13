/* stdf_app_subject_input - 用户输入域 subject */

#ifndef __STDF_APP_SUBJECT_INPUT_H__
#define __STDF_APP_SUBJECT_INPUT_H__

#define STDF_APP_SUBJECTS_INPUT \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_BUTTON_PRESSED,            "button.pressed") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_TOUCH_EVENT,               "touch.event") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_ENCODER_ROTATED,           "encoder.rotated") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_REMOTE_CMD,                "remote.cmd") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_VOICE_CMD,                 "voice.cmd")

#endif
