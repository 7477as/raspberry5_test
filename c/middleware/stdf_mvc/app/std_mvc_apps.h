/* std_mvc_apps - 业务层聚合入口（替代原 std_mvc_subsystems_init/tick） */

#ifndef __STDF_MVC_APPS_H__
#define __STDF_MVC_APPS_H__

#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile sig_atomic_t g_running;

int  app_init(void);
void app_tick(void);
void app_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_MVC_APPS_H__ */
