/* stdf_app - 业务子系统聚合 (按 os -> hal -> bsp -> app 顺序) */

#include "stdf_app.h"
#include "stdf_mvc.h"

#include "stdf_app_hal_dummy.h"
#include "stdf_app_input_mock.h"
#include "stdf_app_imu.h"
#include "stdf_app_battery.h"
#include "stdf_app_network.h"
#include "stdf_app_storage.h"
#include "stdf_app_ai.h"
#include "stdf_app_gimbal.h"
#include "stdf_app_video.h"
#include "stdf_app_indicator.h"

#define STDF_APP_LOG(...)             do { } while (0)

int stdf_app_init(void)
{
    stdf_app_hal_dummy_init();
    stdf_app_input_mock_init();
    stdf_app_imu_init();
    stdf_app_battery_init();
    stdf_app_network_init();
    stdf_app_storage_init();
    stdf_app_ai_init();
    stdf_app_gimbal_init();
    stdf_app_video_init();
    stdf_app_indicator_init();
    return 0;
}

void stdf_app_tick(void)
{
    stdf_app_hal_dummy_tick();

    stdf_app_input_mock_tick();
    stdf_app_imu_tick();
    stdf_app_battery_tick();
    stdf_app_network_tick();
    stdf_app_storage_tick();
    stdf_app_ai_tick();
    stdf_app_gimbal_tick();
    stdf_app_video_tick();
}

void stdf_app_loop(void)
{
    while (1) {
        stdf_app_tick();
    }
}
