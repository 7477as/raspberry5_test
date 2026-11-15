/**
 * @file stdf_mvc_core_port.h
 * @brief stdf mvc core platform abstraction; business code should not include this directly
 */

#ifndef __STDF_MVC_CORE_PORT_H__
#define __STDF_MVC_CORE_PORT_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void   *(*memcpy_fn)(void *dst, const void *src, size_t n);
    void   *(*memset_fn)(void *dst, int c, size_t n);
    void    (*enter_critical)(void);
    void    (*exit_critical)(void);
    uint32_t (*get_tick_ms)(void);
} stdf_mvc_core_port_ops_t;

/* Install the Linux mutex, clock and memory operations before use. */
void stdf_mvc_core_port_install_default_linux(void);
/* Install platform operations before starting publishers or polling. */
void stdf_mvc_core_port_install(const stdf_mvc_core_port_ops_t *ops);
const stdf_mvc_core_port_ops_t *stdf_mvc_core_port_get(void);

void    stdf_mvc_core_port_enter_critical(void);
void    stdf_mvc_core_port_exit_critical(void);
uint32_t stdf_mvc_core_port_get_tick_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_MVC_CORE_PORT_H__ */
