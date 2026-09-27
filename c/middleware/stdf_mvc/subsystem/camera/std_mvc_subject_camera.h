/* std_mvc_subjects_camera - camera 子系统 subject 定义 */

#ifndef __STDF_MVC_SUBJECTS_CAMERA_H__
#define __STDF_MVC_SUBJECTS_CAMERA_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STDF_MVC_SUBJECT_CAMERA_FRAME_RAW = 200,
    STDF_MVC_SUBJECT_CAMERA_FRAME_ENCODED,
    STDF_MVC_SUBJECT_CAMERA_AI_RESULT,
    STDF_MVC_SUBJECT_CAMERA_GIMBAL_STATE,
    STDF_MVC_SUBJECT_CAMERA_COUNT
} std_mvc_subjects_camera_t;

#ifdef __cplusplus
}
#endif

#endif /* __STDF_MVC_SUBJECTS_CAMERA_H__ */
