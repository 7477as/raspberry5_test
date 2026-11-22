/* stdf_app_ui_print - 打印订阅者 */

#ifndef __STDF_APP_UI_PRINT_H__
#define __STDF_APP_UI_PRINT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Subscribe console output to system status; return zero on success. */
int stdf_app_ui_print_init(void);
/* Check status freshness from the main loop. */
void stdf_app_ui_print_poll(void);
/* Unsubscribe console output. */
void stdf_app_ui_print_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_APP_UI_PRINT_H__ */
