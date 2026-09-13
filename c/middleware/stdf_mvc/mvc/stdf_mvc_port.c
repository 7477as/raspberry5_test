/* stdf_mvc_port - 框架对 port ops 的访问层 (非平台相关) */

#include "stdf_mvc_port.h"

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
