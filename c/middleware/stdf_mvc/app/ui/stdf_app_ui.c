/* UI lifecycle and main-thread event dispatch. */
#include "stdf_app_ui.h"
#include "stdf_app_ui_print.h"
#ifdef STDF_MVC_HAS_SDL2
#include "stdf_app_ui_sdl.h"
#endif

int stdf_app_ui_init(void)
{
    if (stdf_app_ui_print_init() != 0) return -1;
#ifdef STDF_MVC_HAS_SDL2
    if (stdf_app_ui_sdl_init() != 0) {
        stdf_app_ui_print_deinit();
        return -1;
    }
#endif
    return 0;
}

void stdf_app_ui_poll(void)
{
    stdf_app_ui_print_poll();
#ifdef STDF_MVC_HAS_SDL2
    stdf_app_ui_sdl_poll();
#endif
}

void stdf_app_ui_deinit(void)
{
#ifdef STDF_MVC_HAS_SDL2
    stdf_app_ui_sdl_deinit();
#endif
    stdf_app_ui_print_deinit();
}
