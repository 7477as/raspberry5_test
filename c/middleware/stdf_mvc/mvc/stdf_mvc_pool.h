/* stdf_mvc_pool - 静态 slot 节点池 */

#ifndef __STDF_MVC_POOL_H__
#define __STDF_MVC_POOL_H__

#include "stdf_mvc_slot.h"

#ifdef __cplusplus
extern "C" {
#endif

void        stdf_mvc_pool_init(void);
stdf_mvc_slot_node_t *stdf_mvc_pool_alloc(void);
void        stdf_mvc_pool_free(stdf_mvc_slot_node_t *node);
uint32_t    stdf_mvc_pool_get_used(void);
uint32_t    stdf_mvc_pool_get_free(void);

#ifdef __cplusplus
}
#endif

#endif
