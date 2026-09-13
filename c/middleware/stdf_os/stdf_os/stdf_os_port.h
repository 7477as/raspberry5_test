/* stdf_os_port - OS 适配层接口（mailbox / timer / mutex / thread），业务侧只看到不透明句柄。
   默认 Linux 实现见 stdf_os_port.c，其他平台未来可提供 stdf_os_port_xxx.c 替换。 */

#ifndef __STDF_OS_PORT_H__
#define __STDF_OS_PORT_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STDF_OS_PORT_WAIT_FOREVER       0xFFFFFFFFu

typedef struct stdf_os_port_mailbox_t   stdf_os_port_mailbox_t;
typedef struct stdf_os_port_timer_t     stdf_os_port_timer_t;
typedef struct stdf_os_port_mutex_t     stdf_os_port_mutex_t;
typedef struct stdf_os_port_thread_t    stdf_os_port_thread_t;

typedef void  (*stdf_os_port_timer_cb_t)(const void *arg);
typedef void *(*stdf_os_port_thread_entry_t)(void *arg);

stdf_os_port_mutex_t  *stdf_os_port_mutex_create(void);
void                   stdf_os_port_mutex_destroy(stdf_os_port_mutex_t *m);
int                    stdf_os_port_mutex_lock(stdf_os_port_mutex_t *m, uint32_t timeout_ms);
int                    stdf_os_port_mutex_unlock(stdf_os_port_mutex_t *m);

stdf_os_port_timer_t  *stdf_os_port_timer_create(stdf_os_port_timer_cb_t cb, void *arg);
int                    stdf_os_port_timer_start(stdf_os_port_timer_t *t, uint32_t delay_ms);
int                    stdf_os_port_timer_stop(stdf_os_port_timer_t *t);
bool                   stdf_os_port_timer_is_running(stdf_os_port_timer_t *t);
void                   stdf_os_port_timer_destroy(stdf_os_port_timer_t *t);

stdf_os_port_mailbox_t *stdf_os_port_mailbox_create(uint32_t capacity, uint32_t slot_size);
void                    stdf_os_port_mailbox_destroy(stdf_os_port_mailbox_t *m);
void                   *stdf_os_port_mailbox_alloc(stdf_os_port_mailbox_t *m, uint32_t timeout_ms);
int                     stdf_os_port_mailbox_put(stdf_os_port_mailbox_t *m, void *block);
void                   *stdf_os_port_mailbox_get(stdf_os_port_mailbox_t *m, uint32_t timeout_ms);
int                     stdf_os_port_mailbox_free(stdf_os_port_mailbox_t *m, void *block);

int                     stdf_os_port_thread_create(stdf_os_port_thread_t **t,
                                                    stdf_os_port_thread_entry_t entry,
                                                    void *arg,
                                                    uint32_t stack_size);
void                    stdf_os_port_thread_destroy(stdf_os_port_thread_t *t);

uint32_t                stdf_os_port_get_current_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_OS_PORT_H__ */
