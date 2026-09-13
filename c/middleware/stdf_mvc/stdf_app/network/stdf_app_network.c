/* stdf_app_network - 网络状态 */

#include "stdf_app_network.h"
#include "stdf_mvc.h"
#include "stdf_app_payload.h"
#include "stdf_app_hal_dummy.h"
#include <string.h>

#define STDF_APP_LOG(...)             do { } while (0)
#define STDF_APP_NETWORK_ASSERT(cond)

int stdf_app_network_init(void)
{
    return 0;
}

void stdf_app_network_tick(void)
{
    const stdf_app_hal_dummy_state_t *hal = stdf_app_hal_dummy_get_state();

    stdf_app_wifi_state_event_t wifi_state = {
        .state = hal->wifi_connected ? WIFI_STATE_CONNECTED : WIFI_STATE_DISCONNECTED,
    };
    strncpy(wifi_state.ssid, "luna-demo", sizeof(wifi_state.ssid) - 1);
    stdf_mvc_signal_data_t p = { .ptr = &wifi_state };
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_WIFI_STATE, &p);

    stdf_app_wifi_rssi_t rssi = { .rssi_dbm = hal->wifi_rssi };
    p.ptr = &rssi;
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_WIFI_RSSI, &p);

    stdf_app_ble_event_t ble = { .state = BLE_STATE_OFF };
    p.ptr = &ble;
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_BLE_STATE, &p);

    stdf_app_rtmp_event_t rtmp = {
        .state = hal->rtmp_live ? RTMP_STATE_LIVE : RTMP_STATE_IDLE,
    };
    strncpy(rtmp.url, "rtmp://demo.luna/live/key", sizeof(rtmp.url) - 1);
    p.ptr = &rtmp;
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_RTMP_STATE, &p);

    stdf_app_mqtt_event_t mqtt = { .state = MQTT_STATE_CONNECTED };
    p.ptr = &mqtt;
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_MQTT_STATE, &p);
}
