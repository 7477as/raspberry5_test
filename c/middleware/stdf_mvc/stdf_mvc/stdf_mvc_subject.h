/* stdf_mvc_subject - X-macro 驱动的 subject id 生成器 */

#ifndef __STDF_MVC_SUBJECT_H__
#define __STDF_MVC_SUBJECT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STDF_MVC_SUBJECT_LIST
#define STDF_MVC_SUBJECT_LIST
#endif

#define STDF_MVC_SUBJECT_X(id, str) id,
typedef enum {
    STDF_MVC_SUBJECT_NONE = 0,
    STDF_MVC_SUBJECT_LIST
    STDF_MVC_SUBJECT_COUNT
} stdf_mvc_subject_id_t;
#undef STDF_MVC_SUBJECT_X

const char *stdf_mvc_subject_str(stdf_mvc_subject_id_t id);
const char *stdf_mvc_subject_name(stdf_mvc_subject_id_t id);

#ifdef __cplusplus
}
#endif

#endif
