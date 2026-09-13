/* stdf_app_hal_dummy - 模拟硬件抽象层 (树莓派 demo, 无真硬件) */

#include "stdf_app_hal_dummy.h"
#include "stdf_mvc.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define STDF_APP_LOG(...)             do { } while (0)
#define STDF_APP_HAL_ASSERT(cond)

static stdf_app_hal_dummy_state_t s_hal_state;
static uint32_t                   s_tick_count = 0;

static float randf(void)
{
    return ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
}

int stdf_app_hal_dummy_init(void)
{
    memset(&s_hal_state, 0, sizeof(s_hal_state));
    s_hal_state.battery_pct   = 87;
    s_hal_state.battery_mv    = 3950;
    s_hal_state.battery_chg   = 0;
    s_hal_state.env_temp_c    = 25.0f;
    s_hal_state.env_humidity_pct = 55.0f;
    s_hal_state.wifi_rssi     = -55;
    s_hal_state.wifi_connected = 1;
    s_hal_state.rtmp_live     = 0;
    s_hal_state.sd_inserted   = 1;
    s_hal_state.sd_remaining_mb = 65536;
    s_hal_state.imu_quat[3]   = 1.0f;
    srand((unsigned)time(NULL));
    return 0;
}

void stdf_app_hal_dummy_tick(void)
{
    s_tick_count++;

    s_hal_state.imu_accel[0] = randf() * 0.5f;
    s_hal_state.imu_accel[1] = randf() * 0.5f;
    s_hal_state.imu_accel[2] = 9.8f + randf() * 0.1f;

    s_hal_state.imu_gyro[0]  = randf() * 0.05f;
    s_hal_state.imu_gyro[1]  = randf() * 0.05f;
    s_hal_state.imu_gyro[2]  = randf() * 0.05f;

    s_hal_state.imu_quat[0]  = randf() * 0.01f;
    s_hal_state.imu_quat[1]  = randf() * 0.01f;
    s_hal_state.imu_quat[2]  = randf() * 0.01f;
    s_hal_state.imu_quat[3]  = 1.0f;

    s_hal_state.imu_temperature_c = 28.0f + randf() * 1.0f;

    if ((s_tick_count % 100) == 0 && s_hal_state.battery_pct > 0) {
        s_hal_state.battery_pct--;
    }

    s_hal_state.battery_mv = 3300 + s_hal_state.battery_pct * 10;

    s_hal_state.wifi_rssi = (int8_t)(-45 - (rand() % 30));

    s_hal_state.env_temp_c += randf() * 0.05f;
    s_hal_state.env_humidity_pct += randf() * 0.2f;
}

const stdf_app_hal_dummy_state_t *stdf_app_hal_dummy_get_state(void)
{
    return &s_hal_state;
}
