/* stdf_mvc latest-event asynchronous delivery. */
#ifndef __STDF_MVC_CORE_ASYNC_H__
#define __STDF_MVC_CORE_ASYNC_H__
#include "stdf_mvc_core_slot.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Reset pending events, drops and subscribers before polling. */
void stdf_mvc_core_async_init(void);
/* Reset delivery state after publishers and polling have stopped. */
void stdf_mvc_core_async_deinit(void);
/* Register callback: 0 success, -1 invalid, -2 duplicate, -3 capacity exhausted. */
int stdf_mvc_core_subject_subscribe(stdf_mvc_publisher_t publisher,
                                   stdf_mvc_core_subject_id_t id,
                                   stdf_mvc_core_slot_fn_t callback);
/* Remove callback; an existing dispatch snapshot may invoke it once more. */
int stdf_mvc_core_subject_unsubscribe(stdf_mvc_publisher_t publisher,
                                     stdf_mvc_core_subject_id_t id,
                                     stdf_mvc_core_slot_fn_t callback);
/* Return the registered subscriber count, or zero for an invalid data subject. */
uint16_t stdf_mvc_core_subject_get_subscriber_count(stdf_mvc_core_subject_id_t id);
/* Copy exactly the registered payload size into the pending event and value cache. */
int stdf_mvc_core_subject_emit_async(stdf_mvc_publisher_t publisher,
                                    stdf_mvc_core_subject_id_t id,
                                    const void *data, uint16_t size);
/* Drain pending events on one owner thread; callbacks may publish but must not poll. */
int stdf_mvc_core_async_poll(void);
/* Return the number of pending events overwritten since initialization. */
uint32_t stdf_mvc_core_async_get_drops(stdf_mvc_core_subject_id_t id);
#ifdef __cplusplus
}
#endif
#endif
