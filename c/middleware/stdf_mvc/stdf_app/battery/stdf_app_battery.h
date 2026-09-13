/* stdf_app_battery - 电池监控 (订阅 level + 充电事件) */

#ifndef __STDF_APP_BATTERY_H__
#define __STDF_APP_BATTERY_H__

#ifdef __cplusplus
extern "C" {
#endif

int stdf_app_battery_init(void);
void stdf_app_battery_tick(void);

#ifdef __cplusplus
}
#endif

#endif
