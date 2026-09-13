/* stdf_mvc_subject_storage - 存储域 subject */

#ifndef __STDF_MVC_SUBJECT_STORAGE_H__
#define __STDF_MVC_SUBJECT_STORAGE_H__

#define STDF_MVC_SUBJECTS_STORAGE \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_SD_CARD_STATE,             "sd.state") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_SD_STORAGE_REMAINING,      "sd.remaining_mb") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_PHOTO_CAPTURED,            "photo.captured") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_FILE_TRANSFER_PROGRESS,    "file.transfer_progress") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_FILE_LIST_CHANGED,         "file.list_changed")

#endif
