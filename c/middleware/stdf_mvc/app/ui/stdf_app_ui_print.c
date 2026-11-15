/* stdf_app_ui_print - 打印订阅者 */

#include "stdf_app_ui_print.h"
#include "util/stdf_mvc_log.h"
#include "stdf_mvc_api.h"
#include "subsystem/dm/stdf_mvc_data_dm.h"
#include <stddef.h>

static float s_last_value = -1000.0f;

static void on_temperature(stdf_mvc_publisher_t publisher,
                           stdf_mvc_subject_id_t subject_id,
                           const void           *payload)
{
    (void)subject_id;
    if (!payload) return;

    const stdf_mvc_dm_temperature_t *s = payload;
    if (s->value_c == s_last_value) return;
    s_last_value = s->value_c;

    STDF_MVC_LOG_I_TAG("[UI]", "%s temp: %.2f C @ %lu ms",
                       publisher == STDF_MVC_LOCAL ? "LOCAL" : "REMOTE",
                       s->value_c, (unsigned long)s->timestamp_ms);
}

int stdf_app_ui_print_init(void)
{
    s_last_value = -1000.0f;
    STDF_MVC_LOG_I_TAG("[UI]", "%s", "print init");
    return stdf_mvc_api_subscribe(STDF_MVC_LOCAL,
                                  STDF_MVC_SUBJECT_DM_TEMPERATURE,
                                  on_temperature);
}
