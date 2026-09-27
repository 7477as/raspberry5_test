/* std_mvc_dm_payloads - data management 业务 payload */

#ifndef __STDF_MVC_DM_PAYLOADS_H__
#define __STDF_MVC_DM_PAYLOADS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float  value_c;
    uint64_t timestamp_ms;
} std_mvc_dm_temperature_t;

#ifdef __cplusplus
}
#endif

#endif /* __STDF_MVC_DM_PAYLOADS_H__ */
