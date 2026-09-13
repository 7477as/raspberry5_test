/* stdf_mvc_port - 平台抽象层，业务代码不应直接引用 */

#ifndef __STDF_MVC_PORT_H__
#define __STDF_MVC_PORT_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*stdf_mvc_post_async_cb_t)(void *payload);

typedef struct {
    void   *(*mem_alloc)(size_t size);
    void    (*mem_free)(void *ptr);
    void   *(*memcpy_fn)(void *dst, const void *src, size_t n);
    void   *(*memset_fn)(void *dst, int c, size_t n);
    void    (*enter_critical)(void);
    void    (*exit_critical)(void);
    uint32_t (*get_tick_ms)(void);
    int     (*post_async)(stdf_mvc_post_async_cb_t cb, void *payload);
} stdf_mvc_port_ops_t;

void stdf_mvc_port_install(const stdf_mvc_port_ops_t *ops);
const stdf_mvc_port_ops_t *stdf_mvc_port_get(void);

void   *stdf_mvc_port_mem_alloc(size_t size);
void    stdf_mvc_port_mem_free(void *ptr);
void    stdf_mvc_port_enter_critical(void);
void    stdf_mvc_port_exit_critical(void);
uint32_t stdf_mvc_port_get_tick_ms(void);
int     stdf_mvc_port_post_async(stdf_mvc_post_async_cb_t cb, void *payload);

#ifdef __cplusplus
}
#endif

#endif
