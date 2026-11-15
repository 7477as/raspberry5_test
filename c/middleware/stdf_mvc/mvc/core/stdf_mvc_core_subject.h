/* stdf_mvc subject identifiers and shared types. */
#ifndef __STDF_MVC_CORE_SUBJECT_H__
#define __STDF_MVC_CORE_SUBJECT_H__
#include <stdint.h>
#include "../subsystem/stdf_mvc_subsystem.h"
#ifdef __cplusplus
extern "C" {
#endif
#define STDF_MVC_SUBJECT_X(id, name) id,
/* Module boundaries preserve existing IDs without exposing numbers in module lists. */
typedef enum stdf_mvc_core_subject_id_t {
    STDF_MVC_SUBJECT_NONE = 0,
    STDF_MVC_DM_SUBJECT_LIST(STDF_MVC_SUBJECT_X)
    STDF_MVC_SUBJECT_DM_END,
    STDF_MVC_SUBJECT_NET_BASE = 99,
    STDF_MVC_NET_SUBJECT_LIST(STDF_MVC_SUBJECT_X)
    STDF_MVC_SUBJECT_NET_END,
    STDF_MVC_SUBJECT_CAMERA_BASE = 199,
    STDF_MVC_CAMERA_SUBJECT_LIST(STDF_MVC_SUBJECT_X)
    STDF_MVC_SUBJECT_CAMERA_END,
    STDF_MVC_SUBJECT_UI_BASE = 299,
    STDF_MVC_UI_SUBJECT_LIST(STDF_MVC_SUBJECT_X)
    STDF_MVC_SUBJECT_COUNT
} stdf_mvc_core_subject_id_t;
#undef STDF_MVC_SUBJECT_X
typedef stdf_mvc_core_subject_id_t stdf_mvc_subject_id_t;
typedef enum stdf_mvc_publisher_t {
    STDF_MVC_LOCAL = 0,
    STDF_MVC_REMOTE = 1,
    STDF_MVC_ANY = 0x7f,
    STDF_MVC_PUBLISHER_LOCAL = STDF_MVC_LOCAL,
    STDF_MVC_PUBLISHER_REMOTE = STDF_MVC_REMOTE,
    STDF_MVC_PUBLISHER_ANY = STDF_MVC_ANY
} stdf_mvc_publisher_t;
typedef enum stdf_mvc_pub_type_t {
    STDF_MVC_PUB_ASYNC = 0,
    STDF_MVC_PUB_SYNC = 1
} stdf_mvc_pub_type_t;
/* Return a subject name, or "invalid" for an unregistered identifier. */
const char *stdf_mvc_core_subject_str(stdf_mvc_core_subject_id_t id);
/* Return the same name as stdf_mvc_core_subject_str. */
const char *stdf_mvc_core_subject_name(stdf_mvc_core_subject_id_t id);
#ifdef __cplusplus
}
#endif
#endif
