/* stdf_mvc per-subject latest-value cache from the supplied core. */
#include "stdf_mvc_core_value.h"
#include "stdf_mvc_core_port.h"
#include <string.h>
#define STDF_MVC_VALUE_STORAGE(id, type) static type g_value_##id;
STDF_MVC_DATA_LIST(STDF_MVC_VALUE_STORAGE)
#undef STDF_MVC_VALUE_STORAGE
typedef struct stdf_mvc_value_slot_t {
    void *bytes;
    size_t size;
} stdf_mvc_value_slot_t;
#define STDF_MVC_VALUE_ENTRY(id, type) [id] = { &g_value_##id, sizeof(type) },
static const stdf_mvc_value_slot_t g_values[STDF_MVC_SUBJECT_COUNT] = {
    STDF_MVC_DATA_LIST(STDF_MVC_VALUE_ENTRY)
};
#undef STDF_MVC_VALUE_ENTRY
static unsigned char g_value_has[STDF_MVC_SUBJECT_COUNT];
void stdf_mvc_core_value_init(void)
{
    stdf_mvc_core_port_enter_critical();
    memset(g_value_has, 0, sizeof(g_value_has));
    stdf_mvc_core_port_exit_critical();
}
void stdf_mvc_core_value_deinit(void)
{
    stdf_mvc_core_value_init();
}
size_t stdf_mvc_core_value_sizeof(stdf_mvc_core_subject_id_t id)
{
    return (unsigned)id < STDF_MVC_SUBJECT_COUNT ? g_values[id].size : 0;
}
int stdf_mvc_core_value_set_locked(stdf_mvc_core_subject_id_t id, const void *data)
{
    if (stdf_mvc_core_value_sizeof(id) == 0 || data == NULL) {
        return -1;
    }
    const stdf_mvc_core_port_ops_t *ops = stdf_mvc_core_port_get();
    void *(*copy_bytes)(void *, const void *, size_t) =
        ops != NULL && ops->memcpy_fn != NULL ? ops->memcpy_fn : memcpy;
    copy_bytes(g_values[id].bytes, data, g_values[id].size);
    g_value_has[id] = 1;
    return 0;
}
int stdf_mvc_core_value_set(stdf_mvc_core_subject_id_t id, const void *data)
{
    stdf_mvc_core_port_enter_critical();
    int result = stdf_mvc_core_value_set_locked(id, data);
    stdf_mvc_core_port_exit_critical();
    return result;
}
int stdf_mvc_core_value_get(stdf_mvc_core_subject_id_t id, void *out)
{
    if (stdf_mvc_core_value_sizeof(id) == 0 || out == NULL) {
        return -1;
    }
    stdf_mvc_core_port_enter_critical();
    int result = g_value_has[id] ? 0 : -2;
    if (result == 0) {
        const stdf_mvc_core_port_ops_t *ops = stdf_mvc_core_port_get();
        void *(*copy_bytes)(void *, const void *, size_t) =
            ops != NULL && ops->memcpy_fn != NULL ? ops->memcpy_fn : memcpy;
        copy_bytes(out, g_values[id].bytes, g_values[id].size);
    }
    stdf_mvc_core_port_exit_critical();
    return result;
}
int stdf_mvc_core_value_has(stdf_mvc_core_subject_id_t id)
{
    if (stdf_mvc_core_value_sizeof(id) == 0) {
        return 0;
    }
    stdf_mvc_core_port_enter_critical();
    int present = g_value_has[id];
    stdf_mvc_core_port_exit_critical();
    return present;
}
