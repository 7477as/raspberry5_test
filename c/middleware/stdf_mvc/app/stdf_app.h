/* stdf_app application lifecycle. */
#ifndef __STDF_APP_H__
#define __STDF_APP_H__

#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile sig_atomic_t g_running;

int stdf_app_init(void);
void stdf_app_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_APP_H__ */
