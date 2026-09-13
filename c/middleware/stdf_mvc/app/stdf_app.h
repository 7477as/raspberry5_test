/* stdf_app - 业务子系统聚合 */

#ifndef __STDF_APP_H__
#define __STDF_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

int stdf_app_init(void);
void stdf_app_tick(void);
void stdf_app_loop(void);

#ifdef __cplusplus
}
#endif

#endif
