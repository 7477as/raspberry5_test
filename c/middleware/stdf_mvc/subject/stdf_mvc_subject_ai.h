/* stdf_mvc_subject_ai - AI 视觉域 subject */

#ifndef __STDF_MVC_SUBJECT_AI_H__
#define __STDF_MVC_SUBJECT_AI_H__

#define STDF_MVC_SUBJECTS_AI \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_AI_DETECTION_RESULT,       "ai.detection_result") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_AI_TRACKING_STATE,         "ai.tracking_state") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_AI_TRACKING_TARGET_CHANGED,"ai.tracking_target") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_AI_FACE_RECOGNIZED,        "ai.face_recognized") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_AI_GESTURE_RECOGNIZED,     "ai.gesture_recognized") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_AI_SCENE_CHANGED,          "ai.scene_changed")

#endif
