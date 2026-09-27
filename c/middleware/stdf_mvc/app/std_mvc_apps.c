/* std_mvc_apps - 业务层聚合入口 */

#include "std_mvc_apps.h"
#include "util/std_mvc_log.h"
#include "dm/std_mvc_dm_temperature.h"
#include "net/std_mvc_net.h"
#include "camera/std_mvc_camera.h"
#include "ui/std_mvc_ui_print.h"
#ifdef STDF_MVC_HAS_SDL2
#include "ui/std_mvc_ui_sdl.h"
#endif

volatile sig_atomic_t g_running = 1;

int app_init(void)
{
    STD_MVC_LOG_I_TAG("[APPS]", "%s", "init");
    int r = 0;
    r |= std_mvc_dm_temperature_init();
    r |= std_mvc_net_init();
    r |= std_mvc_camera_init();
    r |= std_mvc_ui_print_init();
#ifdef STDF_MVC_HAS_SDL2
    r |= std_mvc_ui_sdl_init();
#endif
    return r;
}

void app_tick(void)
{
    std_mvc_dm_temperature_tick();
    std_mvc_net_tick();
    std_mvc_camera_tick();
    std_mvc_ui_print_tick();
#ifdef STDF_MVC_HAS_SDL2
    std_mvc_ui_sdl_tick();
#endif
}

void app_deinit(void)
{
#ifdef STDF_MVC_HAS_SDL2
    std_mvc_ui_sdl_deinit();
#endif
}
