/* Device information application lifecycle. */
#ifndef __STDF_APP_DM_DEVICE_INFO_H__
#define __STDF_APP_DM_DEVICE_INFO_H__
#ifdef __cplusplus
extern "C" {
#endif
/* Start independent device sampling; return zero on success. */
int stdf_app_dm_device_info_init(void);
/* Stop and join device sampling. */
void stdf_app_dm_device_info_deinit(void);
#ifdef __cplusplus
}
#endif
#endif
