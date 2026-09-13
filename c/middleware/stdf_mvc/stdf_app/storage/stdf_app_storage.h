/* stdf_app_storage - 存储管理 (SD 卡状态 + 文件) */

#ifndef __STDF_APP_STORAGE_H__
#define __STDF_APP_STORAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

int stdf_app_storage_init(void);
void stdf_app_storage_tick(void);

#ifdef __cplusplus
}
#endif

#endif
