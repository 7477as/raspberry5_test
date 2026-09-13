/* stdf_app_key_sim - mock 按键模拟器，用于无物理按键环境演示。
   后台线程周期性调用 stdf_app_key_inject_press / stdf_app_key_inject_release，
   模拟单/双/三击/长按四种 pattern。 */

#ifndef __STDF_APP_KEY_SIM_H__
#define __STDF_APP_KEY_SIM_H__

#ifdef __cplusplus
extern "C" {
#endif

void stdf_app_key_sim_init(void);
void stdf_app_key_sim_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_APP_KEY_SIM_H__ */
