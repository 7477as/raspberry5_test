/* stdf_app_input_mock - 模拟用户输入 */

#include "stdf_app_input_mock.h"
#include "stdf_mvc.h"
#include "stdf_mvc_payloads.h"

#define STDF_APP_LOG(...)             do { } while (0)
#define STDF_APP_INPUT_MOCK_ASSERT(cond)

static uint32_t s_tick_count = 0;

int stdf_app_input_mock_init(void)
{
    return 0;
}

void stdf_app_input_mock_tick(void)
{
    s_tick_count++;

    if (s_tick_count == 50) {
        stdf_mvc_payload_button_event_t btn = {
            .id     = STDF_MVC_PAYLOAD_BTN_RECORD,
            .event  = STDF_MVC_PAYLOAD_BTN_EVT_PRESS,
            .hold_ms = 0,
        };
        stdf_mvc_signal_data_t p = { .ptr = &btn };
        stdf_mvc_subject_emit(STDF_MVC_SUBJECT_BUTTON_PRESSED, &p);
    }

    if (s_tick_count == 250) {
        stdf_mvc_payload_button_event_t btn = {
            .id     = STDF_MVC_PAYLOAD_BTN_RECORD,
            .event  = STDF_MVC_PAYLOAD_BTN_EVT_PRESS,
            .hold_ms = 0,
        };
        stdf_mvc_signal_data_t p = { .ptr = &btn };
        stdf_mvc_subject_emit(STDF_MVC_SUBJECT_BUTTON_PRESSED, &p);
    }

    if (s_tick_count == 400) {
        stdf_mvc_payload_button_event_t btn = {
            .id     = STDF_MVC_PAYLOAD_BTN_MODE,
            .event  = STDF_MVC_PAYLOAD_BTN_EVT_PRESS,
            .hold_ms = 0,
        };
        stdf_mvc_signal_data_t p = { .ptr = &btn };
        stdf_mvc_subject_emit(STDF_MVC_SUBJECT_BUTTON_PRESSED, &p);
    }

    if (s_tick_count % 200 == 0) {
        stdf_mvc_payload_encoder_event_t enc = {
            .delta     = 1,
            .direction = 1,
        };
        stdf_mvc_signal_data_t p = { .ptr = &enc };
        stdf_mvc_subject_emit(STDF_MVC_SUBJECT_ENCODER_ROTATED, &p);
    }
}
