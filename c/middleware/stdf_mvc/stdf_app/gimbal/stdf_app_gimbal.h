/* stdf_app_gimbal - 云台控制 (订阅 AI/IMU → 发射姿态) */

#ifndef __STDF_APP_GIMBAL_H__
#define __STDF_APP_GIMBAL_H__

#ifdef __cplusplus
extern "C" {
#endif

int stdf_app_gimbal_init(void);
void stdf_app_gimbal_tick(void);

#ifdef __cplusplus
}
#endif

#endif
