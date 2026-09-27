/* std_mvc_subjects_net - network 子系统 subject 定义 */

#ifndef __STDF_MVC_SUBJECTS_NET_H__
#define __STDF_MVC_SUBJECTS_NET_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STDF_MVC_SUBJECT_NET_WIFI_STATE = 100,
    STDF_MVC_SUBJECT_NET_ETH_STATE,
    STDF_MVC_SUBJECT_NET_MQTT_STATE,
    STDF_MVC_SUBJECT_NET_NTP_SYNC,
    STDF_MVC_SUBJECT_NET_COUNT
} std_mvc_subjects_net_t;

#ifdef __cplusplus
}
#endif

#endif /* __STDF_MVC_SUBJECTS_NET_H__ */
