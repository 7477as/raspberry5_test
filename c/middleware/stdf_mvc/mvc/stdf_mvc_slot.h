/* stdf_mvc_slot - slot 节点定义与回调签名 */

#ifndef __STDF_MVC_SLOT_H__
#define __STDF_MVC_SLOT_H__

#include "stdf_mvc_signal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*stdf_mvc_slot_fn_t)(void *user_data,
                                   const stdf_mvc_signal_data_t *data);

typedef struct stdf_mvc_slot_node {
    stdf_mvc_slot_fn_t          slot_fn;
    void                       *user_data;
    struct stdf_mvc_slot_node  *next;
} stdf_mvc_slot_node_t;

#ifdef __cplusplus
}
#endif

#endif
