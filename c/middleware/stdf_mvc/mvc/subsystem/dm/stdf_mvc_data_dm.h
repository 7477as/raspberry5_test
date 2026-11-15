/* stdf_mvc data management payloads and registration. */
#ifndef __STDF_MVC_DATA_DM_H__
#define __STDF_MVC_DATA_DM_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stdf_mvc_dm_temperature_t {
    float value_c;
    uint64_t timestamp_ms;
} stdf_mvc_dm_temperature_t;

#define STDF_MVC_DM_DATA_LIST(X) \
    X(STDF_MVC_SUBJECT_DM_TEMPERATURE, stdf_mvc_dm_temperature_t)

#ifdef __cplusplus
}
#endif
#endif
