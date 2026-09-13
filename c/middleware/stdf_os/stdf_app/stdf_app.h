/* stdf_app - 业务子系统聚合入口 */

#ifndef __STDF_APP_H__
#define __STDF_APP_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化所有 app 子功能（必须先 stdf_os_init） */
void stdf_app_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_APP_H__ */
