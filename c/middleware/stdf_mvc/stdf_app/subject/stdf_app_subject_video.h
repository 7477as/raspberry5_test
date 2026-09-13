/* stdf_app_subject_video - 视频流域 subject */

#ifndef __STDF_APP_SUBJECT_VIDEO_H__
#define __STDF_APP_SUBJECT_VIDEO_H__

#define STDF_APP_SUBJECTS_VIDEO \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_VIDEO_FRAME_READY,         "video.frame_ready") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_VIDEO_FRAME_ENCODED,       "video.frame_encoded") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_VIDEO_RECORD_STATE,        "video.record_state") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_VIDEO_RECORD_DURATION,     "video.record_duration_ms") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_VIDEO_RESOLUTION_CHANGED,  "video.resolution") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_VIDEO_BITRATE_CHANGED,     "video.bitrate") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_VIDEO_NIGHT_MODE_CHANGED,  "video.night_mode")

#endif
