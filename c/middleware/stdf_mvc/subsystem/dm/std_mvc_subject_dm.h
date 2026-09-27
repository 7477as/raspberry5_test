/* std_mvc_subjects_dm - dm 子系统 subject 定义 */

#ifndef __STDF_MVC_SUBJECTS_DM_H__
#define __STDF_MVC_SUBJECTS_DM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* 顺序定义 subject ID（必须与 subsystem/std_mvc_subsystems.h 的 X-list 顺序一致） */
typedef enum {
    STDF_MVC_SUBJECT_DM_TEMPERATURE = 1,
    STDF_MVC_SUBJECT_DM_HUMIDITY,
    STDF_MVC_SUBJECT_DM_BATTERY,
    STDF_MVC_SUBJECT_DM_STORAGE,
    STDF_MVC_SUBJECT_DM_NETWORK,
    STDF_MVC_SUBJECT_DM_COUNT
} std_mvc_subjects_dm_t;

#ifdef __cplusplus
}
#endif

#endif /* __STDF_MVC_SUBJECTS_DM_H__ */
