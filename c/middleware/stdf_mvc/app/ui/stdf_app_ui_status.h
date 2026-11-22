/* Shared display formatting for independent UI subscribers. */
#ifndef __STDF_APP_UI_STATUS_H__
#define __STDF_APP_UI_STATUS_H__
#include "stdf_mvc_api.h"
#include <stddef.h>
#include <stdint.h>
#define STDF_UI_GROUPS 6u
#define STDF_UI_LINES 48u
#define STDF_UI_LINE_LENGTH 320u

typedef struct stdf_app_ui_status_t {
    uint64_t timestamp_ms;
    unsigned count;
    float bar_percent;
    char lines[STDF_UI_LINES][STDF_UI_LINE_LENGTH];
} stdf_app_ui_status_t;

extern const stdf_mvc_subject_id_t g_ui_subjects[STDF_UI_GROUPS];
extern const char *const g_ui_titles[STDF_UI_GROUPS];
/* Format payload for a registered UI group; copy all text into output. */
void stdf_app_ui_status_format(unsigned group, const void *payload, stdf_app_ui_status_t *output);
/* Return true when a previously received group has exceeded its update deadline. */
int stdf_app_ui_status_stale(unsigned group, uint64_t timestamp_ms, uint64_t now_ms);
#endif
