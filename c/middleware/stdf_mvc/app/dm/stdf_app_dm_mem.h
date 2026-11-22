/* Memory sampling application lifecycle. */
#ifndef __STDF_APP_DM_MEM_H__
#define __STDF_APP_DM_MEM_H__
#ifdef __cplusplus
extern "C" {
#endif
/* Start independent memory sampling; return zero on success. */
int stdf_app_dm_mem_init(void);
/* Stop and join memory sampling. */
void stdf_app_dm_mem_deinit(void);
#ifdef __cplusplus
}
#endif
#endif
