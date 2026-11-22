/* CPU sampling application lifecycle. */
#ifndef __STDF_APP_DM_CPU_H__
#define __STDF_APP_DM_CPU_H__
#ifdef __cplusplus
extern "C" {
#endif
/* Start independent CPU sampling; return zero on success. */
int stdf_app_dm_cpu_init(void);
/* Stop and join CPU sampling. */
void stdf_app_dm_cpu_deinit(void);
#ifdef __cplusplus
}
#endif
#endif
