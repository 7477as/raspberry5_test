/* stdf_mvc - 框架聚合门面 */

#ifndef __STDF_MVC_H__
#define __STDF_MVC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdf_app_subject.h"

#include "stdf_mvc_subject.h"
#include "stdf_mvc_signal.h"
#include "stdf_mvc_slot.h"
#include "stdf_mvc_pool.h"
#include "stdf_mvc_emit.h"
#include "stdf_mvc_async.h"
#include "stdf_mvc_port.h"

int stdf_mvc_init(void);
void stdf_mvc_cleanup(void);

uint32_t stdf_mvc_get_pool_used(void);
uint32_t stdf_mvc_get_pool_free(void);
void     stdf_mvc_dump_subjects(void);

void stdf_mvc_port_install_default_linux(void);
void stdf_mvc_port_install_default_melis(void);

#ifdef __cplusplus
}
#endif

#endif
