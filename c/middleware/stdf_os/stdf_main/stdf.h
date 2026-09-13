/* stdf - 顶层聚合入口声明 */

#ifndef __STDF_H__
#define __STDF_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 按依赖顺序初始化所有子系统 */
void stdf_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_H__ */
