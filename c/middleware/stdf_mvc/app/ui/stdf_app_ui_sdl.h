/* stdf_app_ui_sdl application lifecycle. */

#ifndef __STDF_APP_UI_SDL_H__
#define __STDF_APP_UI_SDL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Create dashboard and subscribe to status; return zero on success. */
int stdf_app_ui_sdl_init(void);
/* Pump events and render status on the main thread. */
void stdf_app_ui_sdl_poll(void);
/* Unsubscribe and release SDL resources. */
void stdf_app_ui_sdl_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_APP_UI_SDL_H__ */
