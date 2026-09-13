/* stdf_mvc_payload_sensor - 传感器域 (电池/环境) payload */

#ifndef __STDF_MVC_PAYLOAD_SENSOR_H__
#define __STDF_MVC_PAYLOAD_SENSOR_H__

#include <stdint.h>

typedef enum {
    STDF_MVC_PAYLOAD_BATTERY_CHG_DISCHG  = 0,
    STDF_MVC_PAYLOAD_BATTERY_CHG_CHARGING= 1,
    STDF_MVC_PAYLOAD_BATTERY_CHG_FULL    = 2,
    STDF_MVC_PAYLOAD_BATTERY_CHG_FAULT   = 3,
} stdf_mvc_payload_battery_chg_state_t;

typedef enum {
    STDF_MVC_PAYLOAD_BATTERY_HEALTH_GOOD = 0,
    STDF_MVC_PAYLOAD_BATTERY_HEALTH_OK   = 1,
    STDF_MVC_PAYLOAD_BATTERY_HEALTH_BAD  = 2,
} stdf_mvc_payload_battery_health_t;

typedef struct {
    uint8_t     level_pct;
} stdf_mvc_payload_battery_level_t;

typedef struct {
    uint16_t    voltage_mv;
} stdf_mvc_payload_battery_voltage_t;

typedef struct {
    stdf_mvc_payload_battery_chg_state_t   state;
} stdf_mvc_payload_battery_chg_event_t;

typedef struct {
    stdf_mvc_payload_battery_health_t      health;
    uint16_t                        cycle_count;
} stdf_mvc_payload_battery_health_event_t;

typedef struct {
    float       temperature_c;
} stdf_mvc_payload_env_temperature_t;

typedef struct {
    float       humidity_pct;
} stdf_mvc_payload_env_humidity_t;

#endif
