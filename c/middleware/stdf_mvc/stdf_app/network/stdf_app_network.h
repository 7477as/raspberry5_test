/* stdf_app_network - 网络状态 (WiFi/BLE/RTMP/MQTT) */

#ifndef __STDF_APP_NETWORK_H__
#define __STDF_APP_NETWORK_H__

#ifdef __cplusplus
extern "C" {
#endif

int stdf_app_network_init(void);
void stdf_app_network_tick(void);

#ifdef __cplusplus
}
#endif

#endif
