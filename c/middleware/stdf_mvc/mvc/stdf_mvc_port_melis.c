/* stdf_mvc_port_melis - 全志 F136 melis RTOS 实现骨架 (占位) */

#include "stdf_mvc_port.h"
#include <stdlib.h>
#include <string.h>

static int s_critical_nesting = 0;
static const stdf_mvc_port_ops_t *s_ops = NULL;

extern uint32_t hal_sys_timer_get(void);
extern void    *os_malloc(uint32_t size);
extern void     os_free(void *ptr);

extern int stdf_mvc_async_queue_push_melis(stdf_mvc_post_async_cb_t cb, void *payload);

static void *melis_mem_alloc(size_t size)
{
    return os_malloc((uint32_t)size);
}

static void melis_mem_free(void *ptr)
{
    os_free(ptr);
}

static void melis_enter_critical(void)
{
    s_critical_nesting++;
}

static void melis_exit_critical(void)
{
    if (s_critical_nesting > 0) {
        s_critical_nesting--;
    }
}

static uint32_t melis_get_tick_ms(void)
{
    return hal_sys_timer_get() / 1000u;
}

static const stdf_mvc_port_ops_t s_melis_ops = {
    .mem_alloc      = melis_mem_alloc,
    .mem_free       = melis_mem_free,
    .memcpy_fn      = memcpy,
    .memset_fn      = memset,
    .enter_critical = melis_enter_critical,
    .exit_critical  = melis_exit_critical,
    .get_tick_ms    = melis_get_tick_ms,
    .post_async     = stdf_mvc_async_queue_push_melis,
};

void stdf_mvc_port_install_default_melis(void)
{
    stdf_mvc_port_install(&s_melis_ops);
}
