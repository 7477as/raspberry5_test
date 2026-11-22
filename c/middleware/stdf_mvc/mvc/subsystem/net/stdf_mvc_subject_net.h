/* stdf_mvc network subjects and names. */
#ifndef __STDF_MVC_SUBJECT_NET_H__
#define __STDF_MVC_SUBJECT_NET_H__

#define STDF_MVC_NET_SUBJECT_LIST(X) \
    X(STDF_MVC_SUBJECT_NET_WIFI_STATE, "net.wifi_state") \
    X(STDF_MVC_SUBJECT_NET_ETH_STATE, "net.eth_state") \
    X(STDF_MVC_SUBJECT_NET_MQTT_STATE, "net.mqtt_state") \
    X(STDF_MVC_SUBJECT_NET_NTP_SYNC, "net.ntp_sync") \
    X(STDF_MVC_SUBJECT_NET_INTERFACES, "net.interfaces")

#endif
