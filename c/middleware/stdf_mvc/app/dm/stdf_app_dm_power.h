/* PMIC sampling application lifecycle. */
#ifndef __STDF_APP_DM_POWER_H__
#define __STDF_APP_DM_POWER_H__
#ifdef __cplusplus
extern "C" {
#endif
/* Start independent PMIC sampling; return zero on success. */
int stdf_app_dm_power_init(void);
/* Stop and join PMIC sampling. */
void stdf_app_dm_power_deinit(void);
#ifdef __cplusplus
}
#endif
#endif
