/* stdf_mvc_payload_network - 网络域 payload */

#ifndef __STDF_MVC_PAYLOAD_NETWORK_H__
#define __STDF_MVC_PAYLOAD_NETWORK_H__

#include <stdint.h>

typedef enum {
    STDF_MVC_PAYLOAD_WIFI_STATE_DISCONNECTED = 0,
    STDF_MVC_PAYLOAD_WIFI_STATE_CONNECTING   = 1,
    STDF_MVC_PAYLOAD_WIFI_STATE_CONNECTED    = 2,
    STDF_MVC_PAYLOAD_WIFI_STATE_AP_MODE      = 3,
} stdf_mvc_payload_wifi_state_t;

typedef enum {
    STDF_MVC_PAYLOAD_BLE_STATE_OFF = 0,
    STDF_MVC_PAYLOAD_BLE_STATE_IDLE,
    STDF_MVC_PAYLOAD_BLE_STATE_ADVERTISING,
    STDF_MVC_PAYLOAD_BLE_STATE_CONNECTED,
} stdf_mvc_payload_ble_state_t;

typedef enum {
    STDF_MVC_PAYLOAD_RTMP_STATE_IDLE      = 0,
    STDF_MVC_PAYLOAD_RTMP_STATE_CONNECTING= 1,
    STDF_MVC_PAYLOAD_RTMP_STATE_LIVE      = 2,
    STDF_MVC_PAYLOAD_RTMP_STATE_ERROR     = 3,
} stdf_mvc_payload_rtmp_state_t;

typedef enum {
    STDF_MVC_PAYLOAD_MQTT_STATE_DISCONNECTED = 0,
    STDF_MVC_PAYLOAD_MQTT_STATE_CONNECTING,
    STDF_MVC_PAYLOAD_MQTT_STATE_CONNECTED,
} stdf_mvc_payload_mqtt_state_t;

typedef struct {
    stdf_mvc_payload_wifi_state_t   state;
    char                    ssid[32];
} stdf_mvc_payload_wifi_state_event_t;

typedef struct {
    int8_t                  rssi_dbm;
} stdf_mvc_payload_wifi_rssi_t;

typedef struct {
    stdf_mvc_payload_ble_state_t    state;
} stdf_mvc_payload_ble_event_t;

typedef struct {
    stdf_mvc_payload_rtmp_state_t   state;
    char                    url[128];
} stdf_mvc_payload_rtmp_event_t;

typedef struct {
    uint32_t                bitrate_bps;
} stdf_mvc_payload_rtmp_bitrate_t;

typedef struct {
    char                    client_ip[32];
    uint32_t                client_id;
} stdf_mvc_payload_rtsp_client_t;

typedef struct {
    stdf_mvc_payload_mqtt_state_t   state;
} stdf_mvc_payload_mqtt_event_t;

typedef struct {
    uint64_t                unix_time_ms;
} stdf_mvc_payload_ntp_event_t;

#endif
