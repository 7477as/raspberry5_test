/* Filesystem capacity publisher for the root mount point. */
#ifndef __STDF_APP_DM_STORAGE_H__
#define __STDF_APP_DM_STORAGE_H__
#ifdef __cplusplus
extern "C" {
#endif
/* Start independent storage sampling; return zero on success. */
int stdf_app_dm_storage_init(void);
/* Stop and join storage sampling. */
void stdf_app_dm_storage_deinit(void);
#ifdef __cplusplus
}
#endif
#endif
