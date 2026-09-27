/* stdf_mvc_api - 公开 API（唯一对业务可见的头） */

#ifndef __STDF_MVC_API_H__
#define __STDF_MVC_API_H__

#include "stdf_mvc_core_types.h"
/* stdf_mvc_subject_id_t defined in core/stdf_mvc_core_subject.h,
   depends on STDF_MVC_SUBJECT_LIST from -include subsystem/std_mvc_subsystems.h */
#include "core/stdf_mvc_core_subject.h"

#ifdef __cplusplus
extern "C" {
#endif

int stdf_mvc_api_subscribe(std_mvc_publisher_t   publisher,
                           stdf_mvc_subject_id_t id,
                           stdf_mvc_slot_fn_t   fn);

int stdf_mvc_api_unsubscribe(std_mvc_publisher_t   publisher,
                             stdf_mvc_subject_id_t id,
                             stdf_mvc_slot_fn_t   fn);

int stdf_mvc_api_publish(std_mvc_publisher_t   publisher,
                          stdf_mvc_subject_id_t id,
                          const void          *data,
                          std_mvc_pub_type_t   pub_type);

int stdf_mvc_api_read(stdf_mvc_subject_id_t id,
                       void                *data);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_MVC_API_H__ */
