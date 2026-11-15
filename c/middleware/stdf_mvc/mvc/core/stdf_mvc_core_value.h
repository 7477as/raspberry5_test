/**
 * @file stdf_mvc_core_value.h
 * @brief per-subject latest-value cache (byte buffer + sizeof lookup)
 *
 * Each subject (STDF_MVC_SUBJECT_*) has exactly one cached payload of size
 * s_value_index[id].size, which equals sizeof(payload type) for that
 * subject. set() copies bytes in; get() copies bytes out; has() reports
 * whether any value has been written since init/last clear.
 *
 * Per-subject storage is sized exactly to sizeof(type) via the X-Macro
 * STDF_MVC_DATA_LIST(); there is no fixed MAX_BYTES cap to bump when a new
 * payload type is added.
 */

#ifndef __STDF_MVC_CORE_VALUE_H__
#define __STDF_MVC_CORE_VALUE_H__

#include <stdint.h>
#include <stddef.h>
#include "stdf_mvc_core_subject.h"
#include "../subsystem/stdf_mvc_subsystem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Write a value while the caller holds the core critical section. */
int stdf_mvc_core_value_set_locked(stdf_mvc_core_subject_id_t id, const void *data);

void   stdf_mvc_core_value_init(void);
void   stdf_mvc_core_value_deinit(void);

/** @brief write latest payload (copies bytes). */
int    stdf_mvc_core_value_set(stdf_mvc_core_subject_id_t id, const void *data);

/** @brief read latest payload into caller buffer (copies bytes). */
int    stdf_mvc_core_value_get(stdf_mvc_core_subject_id_t id, void *out);

/** @brief has any value been written since init/last clear? */
int    stdf_mvc_core_value_has(stdf_mvc_core_subject_id_t id);

/** @brief per-subject payload size in bytes (0 for NONE/invalid). */
size_t stdf_mvc_core_value_sizeof(stdf_mvc_core_subject_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_MVC_CORE_VALUE_H__ */
