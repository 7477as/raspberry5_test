/* stdf_mvc_subject_network - 网络域 subject */

#ifndef __STDF_MVC_SUBJECT_NETWORK_H__
#define __STDF_MVC_SUBJECT_NETWORK_H__

#define STDF_MVC_SUBJECTS_NETWORK \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_WIFI_STATE,                "wifi.state") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_WIFI_RSSI,                 "wifi.rssi_dbm") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_BLE_STATE,                 "ble.state") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_RTMP_STATE,                "rtmp.state") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_RTMP_BITRATE,              "rtmp.bitrate_bps") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_RTSP_CLIENT_CONNECTED,     "rtsp.client") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_MQTT_STATE,                "mqtt.state") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_NTP_TIME_SYNCED,           "ntp.time_synced")

#endif
