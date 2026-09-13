/* stdf_app_imu - IMU 数据源 (从 hal 读取 + 发射 IMU 事件) */

#ifndef __STDF_APP_IMU_H__
#define __STDF_APP_IMU_H__

#ifdef __cplusplus
extern "C" {
#endif

int stdf_app_imu_init(void);
void stdf_app_imu_tick(void);

#ifdef __cplusplus
}
#endif

#endif
