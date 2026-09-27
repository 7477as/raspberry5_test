/* stdf_mvc_api - 公开 API 实现（薄壳，转调 core dispatch） */

#include "stdf_mvc_api.h"
#include "stdf_mvc_core_dispatch.h"

int stdf_mvc_api_subscribe(std_mvc_publisher_t   publisher,
                           stdf_mvc_subject_id_t id,
                           stdf_mvc_slot_fn_t   fn)
{
    return stdf_mvc_core_dispatch_subscribe(publisher, id, fn);
}

int stdf_mvc_api_unsubscribe(std_mvc_publisher_t   publisher,
                             stdf_mvc_subject_id_t id,
                             stdf_mvc_slot_fn_t   fn)
{
    return stdf_mvc_core_dispatch_unsubscribe(publisher, id, fn);
}

int stdf_mvc_api_publish(std_mvc_publisher_t   publisher,
                          stdf_mvc_subject_id_t id,
                          const void          *data,
                          std_mvc_pub_type_t   pub_type)
{
    return stdf_mvc_core_dispatch_publish(publisher, id, data, pub_type);
}

int stdf_mvc_api_read(stdf_mvc_subject_id_t id, void *data)
{
    return stdf_mvc_core_dispatch_read(id, data);
}
