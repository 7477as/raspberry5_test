/* Independent application lifecycles with partial-initialization cleanup. */
#include "stdf_app.h"
#include "dm/stdf_app_dm_cpu.h"
#include "dm/stdf_app_dm_mem.h"
#include "dm/stdf_app_dm_device_info.h"
#include "dm/stdf_app_dm_power.h"
#include "dm/stdf_app_dm_storage.h"
#include "net/stdf_app_net.h"
#include "camera/stdf_app_camera.h"
#include "ui/stdf_app_ui.h"

volatile sig_atomic_t g_running = 1;
static unsigned g_initialized;

enum {
    APP_UI = 1u << 0,
    APP_CPU = 1u << 1,
    APP_MEM = 1u << 2,
    APP_DEVICE = 1u << 3,
    APP_POWER = 1u << 4,
    APP_STORAGE = 1u << 5,
    APP_NET = 1u << 6,
    APP_CAMERA = 1u << 7
};

int stdf_app_init(void)
{
    g_initialized = 0;
    if (stdf_app_ui_init() != 0) goto failure;
    g_initialized |= APP_UI;
    if (stdf_app_dm_cpu_init() != 0) goto failure;
    g_initialized |= APP_CPU;
    if (stdf_app_dm_mem_init() != 0) goto failure;
    g_initialized |= APP_MEM;
    if (stdf_app_dm_device_info_init() != 0) goto failure;
    g_initialized |= APP_DEVICE;
    if (stdf_app_dm_power_init() != 0) goto failure;
    g_initialized |= APP_POWER;
    if (stdf_app_dm_storage_init() != 0) goto failure;
    g_initialized |= APP_STORAGE;
    if (stdf_app_net_init() != 0) goto failure;
    g_initialized |= APP_NET;
    if (stdf_app_camera_init() != 0) goto failure;
    g_initialized |= APP_CAMERA;
    return 0;

failure:
    stdf_app_deinit();
    return -1;
}

void stdf_app_deinit(void)
{
    if (g_initialized & APP_CAMERA) g_initialized &= ~APP_CAMERA;
    if (g_initialized & APP_NET) { stdf_app_net_deinit(); g_initialized &= ~APP_NET; }
    if (g_initialized & APP_STORAGE) { stdf_app_dm_storage_deinit(); g_initialized &= ~APP_STORAGE; }
    if (g_initialized & APP_POWER) { stdf_app_dm_power_deinit(); g_initialized &= ~APP_POWER; }
    if (g_initialized & APP_DEVICE) { stdf_app_dm_device_info_deinit(); g_initialized &= ~APP_DEVICE; }
    if (g_initialized & APP_MEM) { stdf_app_dm_mem_deinit(); g_initialized &= ~APP_MEM; }
    if (g_initialized & APP_CPU) { stdf_app_dm_cpu_deinit(); g_initialized &= ~APP_CPU; }
    if (g_initialized & APP_UI) { stdf_app_ui_deinit(); g_initialized &= ~APP_UI; }
}
