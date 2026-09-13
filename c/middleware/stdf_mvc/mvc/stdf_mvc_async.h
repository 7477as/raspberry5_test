/* stdf_mvc_async - 异步发射 (跨线程投递) */

#ifndef __STDF_MVC_ASYNC_H__
#define __STDF_MVC_ASYNC_H__

#include "stdf_mvc_subject.h"
#include "stdf_mvc_signal.h"

#ifdef __cplusplus
extern "C" {
#endif

void stdf_mvc_async_subsystem_init(void);
void stdf_mvc_async_subsystem_deinit(void);

int  stdf_mvc_subject_emit_async(stdf_mvc_subject_id_t          id,
                                  const stdf_mvc_signal_data_t  *data);

#ifdef __cplusplus
}
#endif

#endif
