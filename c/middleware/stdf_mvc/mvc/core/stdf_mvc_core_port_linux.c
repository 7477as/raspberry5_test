/* stdf_mvc Linux platform operations. */
#include "stdf_mvc_core_port.h"
#include <pthread.h>
#include <string.h>
#include <time.h>
static pthread_mutex_t g_critical_mutex = PTHREAD_MUTEX_INITIALIZER;
static void enter_critical(void)
{
    pthread_mutex_lock(&g_critical_mutex);
}
static void exit_critical(void)
{
    pthread_mutex_unlock(&g_critical_mutex);
}
static uint32_t get_tick_ms(void)
{
    struct timespec timestamp;
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    return (uint32_t)((uint64_t)timestamp.tv_sec * 1000u +
                      (uint64_t)timestamp.tv_nsec / 1000000u);
}
static const stdf_mvc_core_port_ops_t g_linux_ops = {
    .memcpy_fn = memcpy,
    .memset_fn = memset,
    .enter_critical = enter_critical,
    .exit_critical = exit_critical,
    .get_tick_ms = get_tick_ms
};
void stdf_mvc_core_port_install_default_linux(void)
{
    stdf_mvc_core_port_install(&g_linux_ops);
}
