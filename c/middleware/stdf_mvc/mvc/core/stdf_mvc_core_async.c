/* stdf_mvc per-subject latest-event delivery, based on the supplied core. */
#include "stdf_mvc_core_async.h"
#include "stdf_mvc_core_value.h"
#include "stdf_mvc_core_port.h"
#include "stdf_mvc_core_config.h"
#include "stdf_mvc_log.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct stdf_mvc_subscriber_t {
    stdf_mvc_core_slot_fn_t callback;
    stdf_mvc_publisher_t publisher;
} stdf_mvc_subscriber_t;

typedef struct stdf_mvc_event_t {
    void *pending;
    void *dispatch;
    size_t capacity;
    stdf_mvc_publisher_t publisher;
    uint32_t drops;
    stdf_mvc_subscriber_t *subscribers;
} stdf_mvc_event_t;

#define STDF_MVC_EVENT_STORAGE(id, type) \
    _Static_assert(sizeof(type) <= UINT16_MAX, "payload exceeds uint16_t size"); \
    static type g_pending_##id; \
    static type g_dispatch_##id; \
    static stdf_mvc_subscriber_t g_subscribers_##id[STDF_MVC_MAX_SUBS_PER_SUBJECT];
STDF_MVC_DATA_LIST(STDF_MVC_EVENT_STORAGE)
#undef STDF_MVC_EVENT_STORAGE
#define STDF_MVC_EVENT_ENTRY(id, type) \
    [id] = { &g_pending_##id, &g_dispatch_##id, sizeof(type), \
             STDF_MVC_LOCAL, 0, g_subscribers_##id },
static stdf_mvc_event_t g_events[STDF_MVC_SUBJECT_COUNT] = {
    STDF_MVC_DATA_LIST(STDF_MVC_EVENT_ENTRY)
};
#undef STDF_MVC_EVENT_ENTRY
#define STDF_MVC_PENDING_WORDS ((STDF_MVC_SUBJECT_COUNT + 63u) / 64u)
static uint64_t g_pending_bits[STDF_MVC_PENDING_WORDS];

static bool is_data_subject(stdf_mvc_core_subject_id_t id)
{
    return (unsigned)id < STDF_MVC_SUBJECT_COUNT && g_events[id].capacity != 0;
}

static bool is_subscription_publisher(stdf_mvc_publisher_t publisher)
{
    return publisher == STDF_MVC_LOCAL || publisher == STDF_MVC_REMOTE ||
           publisher == STDF_MVC_ANY;
}

static void reset_async_state(void)
{
    stdf_mvc_core_port_enter_critical();
    memset(g_pending_bits, 0, sizeof(g_pending_bits));
    for (unsigned subject = 0; subject < STDF_MVC_SUBJECT_COUNT; subject++) {
        stdf_mvc_event_t *event = &g_events[subject];
        event->drops = 0;
        if (event->subscribers != NULL) {
            memset(event->subscribers, 0,
                   sizeof(*event->subscribers) * STDF_MVC_MAX_SUBS_PER_SUBJECT);
        }
    }
    stdf_mvc_core_port_exit_critical();
}

void stdf_mvc_core_async_init(void)
{
    reset_async_state();
#if STDF_MVC_CORE_ASYNC_INIT_LOG
    STDF_MVC_LOG_I("async init: subjects=%u capacity=%u", (unsigned)STDF_MVC_SUBJECT_COUNT,
                   (unsigned)STDF_MVC_MAX_SUBS_PER_SUBJECT);
#endif
}

void stdf_mvc_core_async_deinit(void)
{
    reset_async_state();
}

int stdf_mvc_core_subject_subscribe(stdf_mvc_publisher_t publisher,
                                   stdf_mvc_core_subject_id_t id,
                                   stdf_mvc_core_slot_fn_t callback)
{
    if (!is_data_subject(id) || callback == NULL || !is_subscription_publisher(publisher)) {
        return -1;
    }
    stdf_mvc_core_port_enter_critical();
    stdf_mvc_subscriber_t *subscribers = g_events[id].subscribers;
    int free_index = -1;
    for (unsigned index = 0; index < STDF_MVC_MAX_SUBS_PER_SUBJECT; index++) {
        if (subscribers[index].callback == callback && subscribers[index].publisher == publisher) {
            stdf_mvc_core_port_exit_critical();
            return -2;
        }
        if (subscribers[index].callback == NULL && free_index < 0) {
            free_index = (int)index;
        }
    }
    if (free_index >= 0) {
        memmove(&subscribers[1], &subscribers[0],
                (size_t)free_index * sizeof(*subscribers));
        subscribers[0].callback = callback;
        subscribers[0].publisher = publisher;
    }
    stdf_mvc_core_port_exit_critical();
#if STDF_MVC_CORE_SUB_CHANGE_LOG
    if (free_index >= 0) {
        STDF_MVC_LOG_I("subscribe [%u] publisher=%d", (unsigned)id, (int)publisher);
    }
#endif
    return free_index < 0 ? -3 : 0;
}

int stdf_mvc_core_subject_unsubscribe(stdf_mvc_publisher_t publisher,
                                     stdf_mvc_core_subject_id_t id,
                                     stdf_mvc_core_slot_fn_t callback)
{
    if (!is_data_subject(id) || callback == NULL || !is_subscription_publisher(publisher)) {
        return -1;
    }
    stdf_mvc_core_port_enter_critical();
    stdf_mvc_subscriber_t *subscribers = g_events[id].subscribers;
    for (unsigned index = 0; index < STDF_MVC_MAX_SUBS_PER_SUBJECT; index++) {
        if (subscribers[index].callback == callback && subscribers[index].publisher == publisher) {
            subscribers[index].callback = NULL;
            stdf_mvc_core_port_exit_critical();
#if STDF_MVC_CORE_SUB_CHANGE_LOG
            STDF_MVC_LOG_I("unsubscribe [%u] publisher=%d", (unsigned)id, (int)publisher);
#endif
            return 0;
        }
    }
    stdf_mvc_core_port_exit_critical();
    return -2;
}

uint16_t stdf_mvc_core_subject_get_subscriber_count(stdf_mvc_core_subject_id_t id)
{
    if (!is_data_subject(id)) {
        return 0;
    }
    uint16_t count = 0;
    stdf_mvc_core_port_enter_critical();
    for (unsigned index = 0; index < STDF_MVC_MAX_SUBS_PER_SUBJECT; index++) {
        count += g_events[id].subscribers[index].callback != NULL;
    }
    stdf_mvc_core_port_exit_critical();
    return count;
}

int stdf_mvc_core_subject_emit_async(stdf_mvc_publisher_t publisher,
                                    stdf_mvc_core_subject_id_t id,
                                    const void *data, uint16_t size)
{
    if (!is_data_subject(id) || data == NULL || size != g_events[id].capacity ||
        (publisher != STDF_MVC_LOCAL && publisher != STDF_MVC_REMOTE)) {
        return -1;
    }
    stdf_mvc_core_port_enter_critical();
    stdf_mvc_event_t *event = &g_events[id];
    uint64_t mask = UINT64_C(1) << ((unsigned)id % 64u);
    if (g_pending_bits[(unsigned)id / 64u] & mask) {
        event->drops++;
    }
    const stdf_mvc_core_port_ops_t *ops = stdf_mvc_core_port_get();
    void *(*copy_bytes)(void *, const void *, size_t) =
        ops != NULL && ops->memcpy_fn != NULL ? ops->memcpy_fn : memcpy;
    copy_bytes(event->pending, data, size);
    stdf_mvc_core_value_set_locked(id, data);
    event->publisher = publisher;
    g_pending_bits[(unsigned)id / 64u] |= mask;
    stdf_mvc_core_port_exit_critical();
    return 0;
}

static int snapshot_subject(unsigned subject, stdf_mvc_core_slot_fn_t *targets,
                            unsigned *target_count, stdf_mvc_publisher_t *publisher)
{
    uint64_t mask = UINT64_C(1) << (subject % 64u);
    if (!(g_pending_bits[subject / 64u] & mask)) {
        return 0;
    }
    stdf_mvc_event_t *event = &g_events[subject];
    g_pending_bits[subject / 64u] &= ~mask;
    const stdf_mvc_core_port_ops_t *ops = stdf_mvc_core_port_get();
    void *(*copy_bytes)(void *, const void *, size_t) =
        ops != NULL && ops->memcpy_fn != NULL ? ops->memcpy_fn : memcpy;
    copy_bytes(event->dispatch, event->pending, event->capacity);
    *publisher = event->publisher;
    *target_count = 0;
    for (unsigned index = 0; index < STDF_MVC_MAX_SUBS_PER_SUBJECT; index++) {
        const stdf_mvc_subscriber_t *subscriber = &event->subscribers[index];
        if (subscriber->callback != NULL &&
            (subscriber->publisher == STDF_MVC_ANY || subscriber->publisher == *publisher)) {
            targets[(*target_count)++] = subscriber->callback;
        }
    }
    return 1;
}

int stdf_mvc_core_async_poll(void)
{
    int dispatched = 0;
    for (;;) {
        stdf_mvc_core_slot_fn_t targets[STDF_MVC_MAX_SUBS_PER_SUBJECT];
        unsigned target_count = 0;
        stdf_mvc_publisher_t publisher = STDF_MVC_LOCAL;
        stdf_mvc_core_port_enter_critical();
        unsigned subject = 1;
        while (subject < STDF_MVC_SUBJECT_COUNT &&
               !(g_pending_bits[subject / 64u] & (UINT64_C(1) << (subject % 64u)))) {
            subject++;
        }
        if (subject == STDF_MVC_SUBJECT_COUNT) {
            stdf_mvc_core_port_exit_critical();
            break;
        }
        snapshot_subject(subject, targets, &target_count, &publisher);
        stdf_mvc_core_port_exit_critical();
        for (unsigned index = 0; index < target_count; index++) {
            targets[index](publisher, (stdf_mvc_core_subject_id_t)subject,
                           g_events[subject].dispatch);
        }
        dispatched++;
    }
    return dispatched;
}

uint32_t stdf_mvc_core_async_get_drops(stdf_mvc_core_subject_id_t id)
{
    if (!is_data_subject(id)) {
        return 0;
    }
    stdf_mvc_core_port_enter_critical();
    uint32_t drops = g_events[id].drops;
    stdf_mvc_core_port_exit_critical();
    return drops;
}
