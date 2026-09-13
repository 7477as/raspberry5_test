/* stdf_app_payload_network - 网络域 payload */

#ifndef __STDF_APP_PAYLOAD_NETWORK_H__
#define __STDF_APP_PAYLOAD_NETWORK_H__

#include <stdint.h>

typedef enum {
    WIFI_STATE_DISCONNECTED = 0,
    WIFI_STATE_CONNECTING   = 1,
    WIFI_STATE_CONNECTED    = 2,
    WIFI_STATE_AP_MODE      = 3,
} stdf_app_wifi_state_t;

typedef enum {
    BLE_STATE_OFF = 0,
    BLE_STATE_IDLE,
    BLE_STATE_ADVERTISING,
    BLE_STATE_CONNECTED,
} stdf_app_ble_state_t;

typedef enum {
    RTMP_STATE_IDLE      = 0,
    RTMP_STATE_CONNECTING= 1,
    RTMP_STATE_LIVE      = 2,
    RTMP_STATE_ERROR     = 3,
} stdf_app_rtmp_state_t;

typedef enum {
    MQTT_STATE_DISCONNECTED = 0,
    MQTT_STATE_CONNECTING,
    MQTT_STATE_CONNECTED,
} stdf_app_mqtt_state_t;

typedef struct {
    stdf_app_wifi_state_t   state;
    char                    ssid[32];
} stdf_app_wifi_state_event_t;

typedef struct {
    int8_t                  rssi_dbm;
} stdf_app_wifi_rssi_t;

typedef struct {
    stdf_app_ble_state_t    state;
} stdf_app_ble_event_t;

typedef struct {
    stdf_app_rtmp_state_t   state;
    char                    url[128];
} stdf_app_rtmp_event_t;

typedef struct {
    uint32_t                bitrate_bps;
} stdf_app_rtmp_bitrate_t;

typedef struct {
    char                    client_ip[32];
    uint32_t                client_id;
} stdf_app_rtsp_client_t;

typedef struct {
    stdf_app_mqtt_state_t   state;
} stdf_app_mqtt_event_t;

typedef struct {
    uint64_t                unix_time_ms;
} stdf_app_ntp_event_t;

#endif
