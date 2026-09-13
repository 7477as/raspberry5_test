/* stdf_mvc_pool - 静态 slot 节点池实现 */

#include "stdf_mvc_pool.h"
#include "stdf_mvc_config.h"

#include <stddef.h>

#define STDF_MVC_LOG_POOL(...)        do { } while (0)
#define STDF_MVC_POOL_ASSERT(cond)

static stdf_mvc_slot_node_t s_pool[STDF_MVC_POOL_SIZE];
static uint32_t             s_pool_used = 0;

void stdf_mvc_pool_init(void)
{
    uint32_t i;
    for (i = 0; i + 1 < STDF_MVC_POOL_SIZE; i++) {
        s_pool[i].next = &s_pool[i + 1];
    }
    s_pool[STDF_MVC_POOL_SIZE - 1].next = NULL;
    s_pool_used = 0;
}

stdf_mvc_slot_node_t *stdf_mvc_pool_alloc(void)
{
    if (s_pool[0].next == NULL) {
        return NULL;
    }
    stdf_mvc_slot_node_t *node = s_pool[0].next;
    s_pool[0].next = node->next;
    s_pool_used++;
    node->slot_fn = NULL;
    node->user_data = NULL;
    node->next = NULL;
    return node;
}

void stdf_mvc_pool_free(stdf_mvc_slot_node_t *node)
{
    if (node == NULL) {
        return;
    }
    node->next = s_pool[0].next;
    s_pool[0].next = node;
    if (s_pool_used > 0) {
        s_pool_used--;
    }
}

uint32_t stdf_mvc_pool_get_used(void)
{
    return s_pool_used;
}

uint32_t stdf_mvc_pool_get_free(void)
{
    return STDF_MVC_POOL_SIZE - s_pool_used;
}
