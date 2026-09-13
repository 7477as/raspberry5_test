/* stdf_mvc_port_linux - 树莓派/Linux 实现 */

#define _POSIX_C_SOURCE 200809L

#include "stdf_mvc_port.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t s_critical_mtx = PTHREAD_MUTEX_INITIALIZER;

static void *linux_mem_alloc(size_t size)
{
    return malloc(size);
}

static void linux_mem_free(void *ptr)
{
    free(ptr);
}

static void linux_enter_critical(void)
{
    pthread_mutex_lock(&s_critical_mtx);
}

static void linux_exit_critical(void)
{
    pthread_mutex_unlock(&s_critical_mtx);
}

static uint32_t linux_get_tick_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

extern int stdf_mvc_async_queue_push_linux(stdf_mvc_post_async_cb_t cb, void *payload);

static const stdf_mvc_port_ops_t s_linux_ops = {
    .mem_alloc      = linux_mem_alloc,
    .mem_free       = linux_mem_free,
    .memcpy_fn      = memcpy,
    .memset_fn      = memset,
    .enter_critical = linux_enter_critical,
    .exit_critical  = linux_exit_critical,
    .get_tick_ms    = linux_get_tick_ms,
    .post_async     = stdf_mvc_async_queue_push_linux,
};

void stdf_mvc_port_install_default_linux(void)
{
    stdf_mvc_port_install(&s_linux_ops);
}
