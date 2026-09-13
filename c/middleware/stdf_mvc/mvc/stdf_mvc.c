/* stdf_mvc - 框架聚合入口 */

#include "stdf_define.h"
#include "stdf_mvc.h"
#include "stdf_mvc_config.h"

int stdf_mvc_init(void)
{
    stdf_mvc_port_install_default_linux();

    stdf_mvc_pool_init();
    stdf_mvc_emit_subsystem_init();
    stdf_mvc_async_subsystem_init();

    STDF_LOG_I("stdf_mvc init ok, subjects=%u pool=%d",
               (unsigned)STDF_MVC_SUBJECT_COUNT, STDF_MVC_POOL_SIZE);
    return 0;
}

void stdf_mvc_deinit(void)
{
    stdf_mvc_async_subsystem_deinit();
    stdf_mvc_emit_subsystem_deinit();
    STDF_LOG_I("%s", "stdf_mvc deinit ok");
}

uint32_t stdf_mvc_get_pool_used(void)
{
    return stdf_mvc_pool_get_used();
}

uint32_t stdf_mvc_get_pool_free(void)
{
    return stdf_mvc_pool_get_free();
}

void stdf_mvc_dump_subjects(void)
{
    stdf_mvc_emit_dump_subjects();
}
