/* stdf_mvc_port - 框架对 port ops 的访问层 (非平台相关) */

#include "stdf_mvc_port.h"
#include "stdf_mvc_config.h"

static const stdf_mvc_port_ops_t *s_port_ops = NULL;

void stdf_mvc_port_install(const stdf_mvc_port_ops_t *ops)
{
    s_port_ops = ops;
}

const stdf_mvc_port_ops_t *stdf_mvc_port_get(void)
{
    return s_port_ops;
}

void *stdf_mvc_port_mem_alloc(size_t size)
{
    if (s_port_ops && s_port_ops->mem_alloc) {
        return s_port_ops->mem_alloc(size);
    }
    return NULL;
}

void stdf_mvc_port_mem_free(void *ptr)
{
    if (s_port_ops && s_port_ops->mem_free) {
        s_port_ops->mem_free(ptr);
    }
}

void stdf_mvc_port_enter_critical(void)
{
    if (s_port_ops && s_port_ops->enter_critical) {
        s_port_ops->enter_critical();
    }
}

void stdf_mvc_port_exit_critical(void)
{
    if (s_port_ops && s_port_ops->exit_critical) {
        s_port_ops->exit_critical();
    }
}

void stdf_mvc_port_log(stdf_mvc_log_lvl_t lvl, const char *fmt, ...)
{
    if (s_port_ops && s_port_ops->log) {
        va_list ap;
        va_start(ap, fmt);
        s_port_ops->log(lvl, fmt, ap);
        va_end(ap);
    }
}

void stdf_mvc_port_assert(int cond, const char *expr, const char *file, int line)
{
#if STDF_MVC_ENABLE_ASSERT
    if (!cond && s_port_ops && s_port_ops->assert_fail) {
        s_port_ops->assert_fail(expr, file, line);
    }
#endif
}

uint32_t stdf_mvc_port_get_tick_ms(void)
{
    if (s_port_ops && s_port_ops->get_tick_ms) {
        return s_port_ops->get_tick_ms();
    }
    return 0;
}

int stdf_mvc_port_post_async(stdf_mvc_post_async_cb_t cb, void *payload)
{
    if (s_port_ops && s_port_ops->post_async) {
        return s_port_ops->post_async(cb, payload);
    }
    return -1;
}
