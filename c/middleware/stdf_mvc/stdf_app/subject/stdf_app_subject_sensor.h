/* stdf_app_subject_sensor - 传感器域 (电池/温湿度) subject */

#ifndef __STDF_APP_SUBJECT_SENSOR_H__
#define __STDF_APP_SUBJECT_SENSOR_H__

#define STDF_APP_SUBJECTS_SENSOR \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_BATTERY_LEVEL,             "battery.level_pct") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_BATTERY_VOLTAGE,           "battery.voltage_mv") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_BATTERY_CHARGING_STATE,    "battery.charging_state") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_BATTERY_HEALTH,            "battery.health") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_ENV_TEMPERATURE,           "env.temperature") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_ENV_HUMIDITY,              "env.humidity")

#endif
