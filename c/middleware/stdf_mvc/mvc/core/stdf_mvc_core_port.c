/**
 * @file stdf_mvc_core_port.c
 * @brief stdf mvc core port ops accessor layer
 */

#include "stdf_mvc_core_port.h"

static const stdf_mvc_core_port_ops_t *g_port_ops = NULL;

void stdf_mvc_core_port_install(const stdf_mvc_core_port_ops_t *ops)
{
    g_port_ops = ops;
}

const stdf_mvc_core_port_ops_t *stdf_mvc_core_port_get(void)
{
    return g_port_ops;
}

void stdf_mvc_core_port_enter_critical(void)
{
    if (g_port_ops && g_port_ops->enter_critical) {
        g_port_ops->enter_critical();
    }
}

void stdf_mvc_core_port_exit_critical(void)
{
    if (g_port_ops && g_port_ops->exit_critical) {
        g_port_ops->exit_critical();
    }
}

uint32_t stdf_mvc_core_port_get_tick_ms(void)
{
    if (g_port_ops && g_port_ops->get_tick_ms) {
        return g_port_ops->get_tick_ms();
    }
    return 0;
}
