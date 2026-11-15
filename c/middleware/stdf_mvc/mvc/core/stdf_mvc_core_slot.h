/**
 * @file stdf_mvc_core_slot.h
 * @brief stdf mvc core slot node definition and callback signature
 */

#ifndef __STDF_MVC_CORE_SLOT_H__
#define __STDF_MVC_CORE_SLOT_H__

#include "stdf_mvc_core_subject.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*stdf_mvc_core_slot_fn_t)(stdf_mvc_publisher_t       publisher,
                                        stdf_mvc_core_subject_id_t subject_id,
                                        const void                *data);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_MVC_CORE_SLOT_H__ */