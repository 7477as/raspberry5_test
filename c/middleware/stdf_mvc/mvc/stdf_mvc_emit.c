/* stdf_mvc_emit - 订阅 / 退订 / 同步发射 (含哨兵节点 + emit 计数) */

#include "stdf_define.h"
#include "stdf_mvc_emit.h"
#include "stdf_mvc_pool.h"
#include "stdf_mvc_port.h"
#include "stdf_mvc_config.h"
#include <stddef.h>

static stdf_mvc_slot_node_t *s_heads[STDF_MVC_SUBJECT_COUNT];
static uint16_t              s_counts[STDF_MVC_SUBJECT_COUNT];

#if STDF_MVC_EMIT_COUNT
static uint64_t              s_emit_counts[STDF_MVC_SUBJECT_COUNT];
#endif

static stdf_mvc_slot_node_t *s_sentinel_pool(void)
{
    stdf_mvc_slot_node_t *node = stdf_mvc_pool_alloc();
    if (node) {
        node->slot_fn = NULL;
        node->user_data = NULL;
        node->next = NULL;
    }
    return node;
}

void stdf_mvc_emit_subsystem_init(void)
{
    uint32_t i;
    for (i = 0; i < STDF_MVC_SUBJECT_COUNT; i++) {
        s_heads[i] = s_sentinel_pool();
        s_counts[i] = 0;
#if STDF_MVC_EMIT_COUNT
        s_emit_counts[i] = 0;
#endif
    }
}

void stdf_mvc_emit_subsystem_deinit(void)
{
    uint32_t i;
    for (i = 0; i < STDF_MVC_SUBJECT_COUNT; i++) {
        if (s_heads[i]) {
            stdf_mvc_pool_free(s_heads[i]);
            s_heads[i] = NULL;
        }
        s_counts[i] = 0;
#if STDF_MVC_EMIT_COUNT
        s_emit_counts[i] = 0;
#endif
    }
}

int stdf_mvc_subject_subscribe(stdf_mvc_subject_id_t id,
                                stdf_mvc_slot_fn_t    fn,
                                void                 *user_data)
{
    if (id >= STDF_MVC_SUBJECT_COUNT || fn == NULL) {
        return -1;
    }

    stdf_mvc_port_enter_critical();

    stdf_mvc_slot_node_t *n = s_heads[id]->next;
    while (n) {
        if (n->slot_fn == fn && n->user_data == user_data) {
            stdf_mvc_port_exit_critical();
            return -2;
        }
        n = n->next;
    }

    stdf_mvc_slot_node_t *node = stdf_mvc_pool_alloc();
    if (!node) {
        stdf_mvc_port_exit_critical();
        STDF_LOG_E("pool full, subscribe [%u] %s failed",
                   (unsigned)id, stdf_mvc_subject_str(id));
        return -3;
    }

    node->slot_fn = fn;
    node->user_data = user_data;
    node->next = s_heads[id]->next;
    s_heads[id]->next = node;
    s_counts[id]++;

    stdf_mvc_port_exit_critical();

#if STDF_MVC_SUB_CHANGE_LOG
    STDF_LOG_D("subscribe [%u] %s -> count=%u",
               (unsigned)id, stdf_mvc_subject_str(id), (unsigned)s_counts[id]);
#endif

    return 0;
}

int stdf_mvc_subject_unsubscribe(stdf_mvc_subject_id_t id,
                                  stdf_mvc_slot_fn_t    fn,
                                  void                 *user_data)
{
    if (id >= STDF_MVC_SUBJECT_COUNT) {
        return -1;
    }

    stdf_mvc_port_enter_critical();

    stdf_mvc_slot_node_t **pp = &s_heads[id]->next;
    while (*pp) {
        if ((*pp)->slot_fn == fn && (*pp)->user_data == user_data) {
            stdf_mvc_slot_node_t *victim = *pp;
            *pp = victim->next;
            stdf_mvc_pool_free(victim);
            s_counts[id]--;
            stdf_mvc_port_exit_critical();

#if STDF_MVC_SUB_CHANGE_LOG
            STDF_LOG_D("unsubscribe [%u] %s -> count=%u",
                       (unsigned)id, stdf_mvc_subject_str(id), (unsigned)s_counts[id]);
#endif
            return 0;
        }
        pp = &(*pp)->next;
    }

    stdf_mvc_port_exit_critical();
    return -2;
}

int stdf_mvc_subject_emit(stdf_mvc_subject_id_t          id,
                           const stdf_mvc_signal_data_t  *data)
{
    if (id >= STDF_MVC_SUBJECT_COUNT || data == NULL) {
        return -1;
    }

    stdf_mvc_port_enter_critical();
    stdf_mvc_slot_node_t *head = s_heads[id]->next;
    uint16_t count = s_counts[id];
    stdf_mvc_port_exit_critical();

#if STDF_MVC_EMIT_COUNT
    s_emit_counts[id]++;
#endif

    stdf_mvc_slot_node_t *cur = head;
    while (cur) {
        stdf_mvc_slot_node_t *next = cur->next;
        if (cur->slot_fn) {
            cur->slot_fn(cur->user_data, data);
        }
        cur = next;
    }
    return (int)count;
}

uint16_t stdf_mvc_subject_get_subscriber_count(stdf_mvc_subject_id_t id)
{
    if (id >= STDF_MVC_SUBJECT_COUNT) {
        return 0;
    }
    return s_counts[id];
}

void stdf_mvc_emit_dump_subjects(void)
{
    STDF_LOG_I("%s", "=== stdf_mvc subjects ===");
    for (stdf_mvc_subject_id_t id = 0; id < STDF_MVC_SUBJECT_COUNT; id++) {
        if (s_counts[id] > 0) {
            STDF_LOG_I("  [%u] %s: %u subs",
                       (unsigned)id, stdf_mvc_subject_str(id), (unsigned)s_counts[id]);
        }
    }
    STDF_LOG_I("=== pool: %u/%u used ===",
               (unsigned)stdf_mvc_pool_get_used(),
               (unsigned)(stdf_mvc_pool_get_used() + stdf_mvc_pool_get_free()));
}
