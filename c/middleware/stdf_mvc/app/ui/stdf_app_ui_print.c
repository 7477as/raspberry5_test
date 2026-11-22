/* Console subscriber prints only the lines that changed. */
#include "stdf_app_ui_print.h"
#include "stdf_app_ui_status.h"
#include "app/stdf_app_sampler.h"
#include <string.h>

static stdf_app_ui_status_t g_previous[STDF_UI_GROUPS];
static bool g_subscribed[STDF_UI_GROUPS];
static bool g_stale[STDF_UI_GROUPS];

static void on_status(stdf_mvc_publisher_t publisher, stdf_mvc_subject_id_t subject,
                      const void *payload)
{
    (void)publisher;
    if (!payload) return;
    for (unsigned group = 0; group < STDF_UI_GROUPS; group++) {
        if (subject != g_ui_subjects[group]) continue;
        stdf_app_ui_status_t current;
        stdf_app_ui_status_format(group, payload, &current);
        bool first = g_previous[group].count == 0 && g_previous[group].timestamp_ms == 0;
        for (unsigned line = 0; line < current.count; line++) {
            if (!first && line < g_previous[group].count &&
                strcmp(current.lines[line], g_previous[group].lines[line]) == 0) continue;
            STDF_MVC_LOG_I_TAG("[UI]", "%s %s", g_ui_titles[group], current.lines[line]);
        }
        g_previous[group] = current;
        g_stale[group] = false;
    }
}

int stdf_app_ui_print_init(void)
{
    memset(g_previous, 0, sizeof(g_previous));
    memset(g_stale, 0, sizeof(g_stale));
    for (unsigned group = 0; group < STDF_UI_GROUPS; group++) {
        int result = stdf_mvc_api_subscribe(STDF_MVC_LOCAL, g_ui_subjects[group], on_status);
        if (result != 0) { stdf_app_ui_print_deinit(); return result; }
        g_subscribed[group] = true;
    }
    return 0;
}

void stdf_app_ui_print_poll(void)
{
    uint64_t now = stdf_app_now_ms();
    for (unsigned group = 0; group < STDF_UI_GROUPS; group++) {
        if (!g_stale[group] && stdf_app_ui_status_stale(group, g_previous[group].timestamp_ms, now)) {
            STDF_MVC_LOG_W_TAG("[UI]", "%s data stale", g_ui_titles[group]);
            g_stale[group] = true;
        }
    }
}

void stdf_app_ui_print_deinit(void)
{
    for (unsigned group = 0; group < STDF_UI_GROUPS; group++) {
        if (g_subscribed[group])
            stdf_mvc_api_unsubscribe(STDF_MVC_LOCAL, g_ui_subjects[group], on_status);
        g_subscribed[group] = false;
    }
}
