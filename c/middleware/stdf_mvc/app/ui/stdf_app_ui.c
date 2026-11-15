/* stdf_app_ui aggregates application UI lifecycle. */
#include "stdf_app_ui.h"
#include "stdf_app_ui_print.h"
#ifdef STDF_MVC_HAS_SDL2
#include "stdf_app_ui_sdl.h"
#endif

int stdf_app_ui_init(void)
{
    if (stdf_app_ui_print_init() != 0) {
        return -1;
    }
#ifdef STDF_MVC_HAS_SDL2
    return stdf_app_ui_sdl_init();
#else
    return 0;
#endif
}

void stdf_app_ui_deinit(void)
{
#ifdef STDF_MVC_HAS_SDL2
    stdf_app_ui_sdl_deinit();
#endif
}
