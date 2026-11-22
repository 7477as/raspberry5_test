/* stdf_app_ui lifecycle. */
#ifndef __STDF_APP_UI_H__
#define __STDF_APP_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize UI subscribers; return zero on success. */
int stdf_app_ui_init(void);
/* Pump UI events and freshness checks. */
void stdf_app_ui_poll(void);
/* Release UI resources and subscriptions. */
void stdf_app_ui_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_APP_UI_H__ */
