/* stdf_app_hal_dummy - 树莓派 demo 用模拟硬件抽象 */

#ifndef __STDF_APP_HAL_DUMMY_H__
#define __STDF_APP_HAL_DUMMY_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float       imu_accel[3];
    float       imu_gyro[3];
    float       imu_quat[4];
    float       imu_temperature_c;
    uint8_t     battery_pct;
    uint16_t    battery_mv;
    uint8_t     battery_chg;
    float       env_temp_c;
    float       env_humidity_pct;
    int8_t      wifi_rssi;
    uint8_t     wifi_connected;
    uint8_t     rtmp_live;
    uint64_t    sd_remaining_mb;
    uint8_t     sd_inserted;
} stdf_app_hal_dummy_state_t;

int      stdf_app_hal_dummy_init(void);
void     stdf_app_hal_dummy_tick(void);
const stdf_app_hal_dummy_state_t *stdf_app_hal_dummy_get_state(void);

#ifdef __cplusplus
}
#endif

#endif
