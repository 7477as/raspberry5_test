/* std_mvc_subjects_ui - ui 子系统 subject 定义 */

#ifndef __STDF_MVC_SUBJECTS_UI_H__
#define __STDF_MVC_SUBJECTS_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STDF_MVC_SUBJECT_UI_BUTTON_PRESSED = 300,
    STDF_MVC_SUBJECT_UI_SCREEN_TOUCH,
    STDF_MVC_SUBJECT_UI_DISPLAY_UPDATE,
    STDF_MVC_SUBJECT_UI_COUNT
} std_mvc_subjects_ui_t;

#ifdef __cplusplus
}
#endif

#endif /* __STDF_MVC_SUBJECTS_UI_H__ */
