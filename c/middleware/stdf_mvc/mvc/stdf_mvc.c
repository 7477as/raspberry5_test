/* stdf_mvc middleware initialization and teardown. */
#include "stdf_mvc.h"
#include "core/stdf_mvc_core.h"

int stdf_mvc_init(void)
{
    return stdf_mvc_core_init();
}

void stdf_mvc_deinit(void)
{
    stdf_mvc_core_deinit();
}
