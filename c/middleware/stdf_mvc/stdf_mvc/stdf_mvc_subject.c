/* stdf_mvc_subject - subject id <-> 名字查表 */

#include "stdf_app_subject.h"
#include "stdf_mvc_subject.h"

#define STDF_MVC_SUBJECT_X(id, str) [id] = str,
static const char *s_subject_str[STDF_MVC_SUBJECT_COUNT] = {
    [STDF_MVC_SUBJECT_NONE] = "none",
    STDF_MVC_SUBJECT_LIST
};
#undef STDF_MVC_SUBJECT_X

const char *stdf_mvc_subject_str(stdf_mvc_subject_id_t id)
{
    if (id >= STDF_MVC_SUBJECT_COUNT) {
        return "invalid";
    }
    return s_subject_str[id];
}

const char *stdf_mvc_subject_name(stdf_mvc_subject_id_t id)
{
    return stdf_mvc_subject_str(id);
}
