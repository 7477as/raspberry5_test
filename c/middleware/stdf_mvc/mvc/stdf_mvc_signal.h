/* stdf_mvc_signal - 通用信号 payload 容器 */

#ifndef __STDF_MVC_SIGNAL_H__
#define __STDF_MVC_SIGNAL_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef union {
    void       *ptr;
    int         i;
    int32_t     i32;
    uint32_t    u32;
    int64_t     i64;
    uint64_t    u64;
    float       f;
    double      d;
    uint8_t     bytes[8];
} stdf_mvc_signal_data_t;

#ifdef __cplusplus
}
#endif

#endif
