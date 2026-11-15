/* stdf_mvc subject name lookup. */
#include "stdf_mvc_core_subject.h"
#include <stddef.h>
#define STDF_MVC_SUBJECT_X(id, name) [id] = name,
static const char *const g_subject_names[STDF_MVC_SUBJECT_COUNT] = {
    [STDF_MVC_SUBJECT_NONE] = "none",
    STDF_MVC_SUBJECT_LIST(STDF_MVC_SUBJECT_X)
};
#undef STDF_MVC_SUBJECT_X
const char *stdf_mvc_core_subject_str(stdf_mvc_core_subject_id_t id)
{
    if ((unsigned)id >= STDF_MVC_SUBJECT_COUNT || g_subject_names[id] == NULL) {
        return "invalid";
    }
    return g_subject_names[id];
}
const char *stdf_mvc_core_subject_name(stdf_mvc_core_subject_id_t id)
{
    return stdf_mvc_core_subject_str(id);
}
