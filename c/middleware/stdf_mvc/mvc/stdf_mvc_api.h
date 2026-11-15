/* stdf_mvc public application interface. */
#ifndef __STDF_MVC_API_H__
#define __STDF_MVC_API_H__
#include "core/stdf_mvc_core_slot.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef stdf_mvc_core_slot_fn_t stdf_mvc_slot_fn_t;
/* Subscribe: 0 success, -1 invalid, -2 duplicate, -3 subject capacity exhausted. */
int stdf_mvc_api_subscribe(stdf_mvc_publisher_t publisher, stdf_mvc_subject_id_t id,
                           stdf_mvc_slot_fn_t callback);
/* Unsubscribe: an already snapshotted callback may still run once. */
int stdf_mvc_api_unsubscribe(stdf_mvc_publisher_t publisher, stdf_mvc_subject_id_t id,
                             stdf_mvc_slot_fn_t callback);
/* Copy a typed payload to the latest-value cache and pending slot; sync returns -2. */
int stdf_mvc_api_publish(stdf_mvc_publisher_t publisher, stdf_mvc_subject_id_t id,
                         const void *data, stdf_mvc_pub_type_t pub_type);
/* Copy the latest typed payload to data; -2 means no value has been published. */
int stdf_mvc_api_read(stdf_mvc_subject_id_t id, void *data);
#ifdef __cplusplus
}
#endif
#endif
