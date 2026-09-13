/* stdf_mvc_emit - 订阅 / 退订 / 同步发射 */

#ifndef __STDF_MVC_EMIT_H__
#define __STDF_MVC_EMIT_H__

#include "stdf_mvc_subject.h"
#include "stdf_mvc_signal.h"
#include "stdf_mvc_slot.h"

#ifdef __cplusplus
extern "C" {
#endif

void     stdf_mvc_emit_subsystem_init(void);
void     stdf_mvc_emit_subsystem_deinit(void);

int      stdf_mvc_subject_subscribe(stdf_mvc_subject_id_t      id,
                                    stdf_mvc_slot_fn_t         fn,
                                    void                      *user_data);
int      stdf_mvc_subject_unsubscribe(stdf_mvc_subject_id_t      id,
                                      stdf_mvc_slot_fn_t         fn,
                                      void                      *user_data);

int      stdf_mvc_subject_emit(stdf_mvc_subject_id_t          id,
                                const stdf_mvc_signal_data_t  *data);

uint16_t stdf_mvc_subject_get_subscriber_count(stdf_mvc_subject_id_t id);

void     stdf_mvc_emit_dump_subjects(void);

#ifdef __cplusplus
}
#endif

#endif
