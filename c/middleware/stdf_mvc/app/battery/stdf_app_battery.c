/* stdf_app_battery - 电池监控 */

#include "stdf_app_battery.h"
#include "stdf_mvc.h"
#include "stdf_mvc_payloads.h"
#include "stdf_app_hal_dummy.h"

#define STDF_APP_LOG(...)             do { } while (0)
#define STDF_APP_BATTERY_ASSERT(cond)

static uint32_t s_last_low_battery_warn_tick = 0;
static uint32_t s_tick_count = 0;

static void on_battery_low_check(void *user_data, const stdf_mvc_signal_data_t *data);

int stdf_app_battery_init(void)
{
    stdf_mvc_subject_subscribe(STDF_MVC_SUBJECT_BATTERY_LEVEL,
                               on_battery_low_check, NULL);
    return 0;
}

static void on_battery_low_check(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    stdf_mvc_payload_battery_level_t *lvl = (stdf_mvc_payload_battery_level_t *)data->ptr;
    if (!lvl) {
        return;
    }
    if (lvl->level_pct <= 15 && (s_tick_count - s_last_low_battery_warn_tick) > 200) {
        stdf_mvc_payload_sys_err_t err = {
            .code = STDF_MVC_PAYLOAD_SYS_ERR_LOW_BATTERY,
            .detail = "battery <= 15%",
            .timestamp_ms = s_tick_count * 10,
        };
        stdf_mvc_signal_data_t p = { .ptr = &err };
        stdf_mvc_subject_emit_async(STDF_MVC_SUBJECT_ERROR_REPORTED, &p);
        s_last_low_battery_warn_tick = s_tick_count;
    }
}

void stdf_app_battery_tick(void)
{
    s_tick_count++;
    const stdf_app_hal_dummy_state_t *hal = stdf_app_hal_dummy_get_state();

    stdf_mvc_payload_battery_level_t lvl = { .level_pct = hal->battery_pct };
    stdf_mvc_signal_data_t p = { .ptr = &lvl };
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_BATTERY_LEVEL, &p);

    stdf_mvc_payload_battery_voltage_t volt = { .voltage_mv = hal->battery_mv };
    p.ptr = &volt;
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_BATTERY_VOLTAGE, &p);

    stdf_mvc_payload_battery_chg_event_t chg = {
        .state = (stdf_mvc_payload_battery_chg_state_t)hal->battery_chg,
    };
    p.ptr = &chg;
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_BATTERY_CHARGING_STATE, &p);
}
