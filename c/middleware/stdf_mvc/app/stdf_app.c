/* stdf_app application lifecycle and internal event loop. */
#include "stdf_app.h"
#include "util/stdf_mvc_log.h"
#include "dm/stdf_app_dm_temperature.h"
#include "net/stdf_app_net.h"
#include "camera/stdf_app_camera.h"
#include "ui/stdf_app_ui.h"

volatile sig_atomic_t g_running = 1;

int stdf_app_init(void)
{
    STDF_MVC_LOG_I_TAG("[APP]", "%s", "init");

    if (stdf_app_net_init() != 0) {
        return -1;
    }

    if (stdf_app_dm_temperature_init() != 0) {
        return -1;
    }

    if (stdf_app_camera_init() != 0) {
        return -1;
    }

    if (stdf_app_ui_init() != 0) {
        return -1;
    }

    return 0;
}

void stdf_app_deinit(void)
{
    stdf_app_dm_temperature_deinit();
    stdf_app_ui_deinit();
}
