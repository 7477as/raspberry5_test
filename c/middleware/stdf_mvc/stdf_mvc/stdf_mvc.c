/* stdf_mvc - 框架聚合入口 */

#include "stdf_mvc.h"
#include "stdf_mvc_config.h"

#define STDF_MVC_LOG_CORE(...)         do { } while (0)

int stdf_mvc_init(void)
{
    stdf_mvc_port_install_default_linux();

    stdf_mvc_pool_init();
    stdf_mvc_emit_subsystem_init();
    stdf_mvc_async_subsystem_init();

    stdf_mvc_port_log(STDF_MVC_LOG_LVL_INFO,
                      "stdf_mvc init ok, subjects=%u pool=%d",
                      (unsigned)STDF_MVC_SUBJECT_COUNT,
                      STDF_MVC_POOL_SIZE);
    return 0;
}

void stdf_mvc_cleanup(void)
{
    stdf_mvc_async_subsystem_cleanup();
    stdf_mvc_emit_subsystem_cleanup();
    stdf_mvc_port_log(STDF_MVC_LOG_LVL_INFO, "stdf_mvc cleanup ok");
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
