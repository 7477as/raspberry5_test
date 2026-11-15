/* stdf_mvc core initialization and teardown. */
#include "stdf_mvc_core.h"
#include "stdf_mvc_core_async.h"
#include "stdf_mvc_core_value.h"
#include "stdf_mvc_core_port.h"

int stdf_mvc_core_init(void)
{
    stdf_mvc_core_port_install_default_linux();
    stdf_mvc_core_value_init();
    stdf_mvc_core_async_init();
    return 0;
}

void stdf_mvc_core_deinit(void)
{
    stdf_mvc_core_async_deinit();
    stdf_mvc_core_value_deinit();
}
