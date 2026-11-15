/* stdf_mvc public interface delegates to the core. */
#include "stdf_mvc_api.h"
#include "core/stdf_mvc_core_async.h"
#include "core/stdf_mvc_core_value.h"
int stdf_mvc_api_subscribe(stdf_mvc_publisher_t publisher, stdf_mvc_subject_id_t id,
                           stdf_mvc_slot_fn_t callback)
{
    return stdf_mvc_core_subject_subscribe(publisher, id, callback);
}
int stdf_mvc_api_unsubscribe(stdf_mvc_publisher_t publisher, stdf_mvc_subject_id_t id,
                             stdf_mvc_slot_fn_t callback)
{
    return stdf_mvc_core_subject_unsubscribe(publisher, id, callback);
}
int stdf_mvc_api_publish(stdf_mvc_publisher_t publisher, stdf_mvc_subject_id_t id,
                         const void *data, stdf_mvc_pub_type_t pub_type)
{
    if (pub_type != STDF_MVC_PUB_ASYNC) {
        return -2;
    }
    size_t size = stdf_mvc_core_value_sizeof(id);
    if (size == 0 || size > UINT16_MAX) {
        return -1;
    }
    return stdf_mvc_core_subject_emit_async(publisher, id, data, (uint16_t)size);
}
int stdf_mvc_api_read(stdf_mvc_subject_id_t id, void *data)
{
    return stdf_mvc_core_value_get(id, data);
}