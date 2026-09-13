/* stdf_app_payload_input - 用户输入域 payload */

#ifndef __STDF_APP_PAYLOAD_INPUT_H__
#define __STDF_APP_PAYLOAD_INPUT_H__

#include <stdint.h>

typedef enum {
    BTN_RECORD     = 0,
    BTN_SHUTTER    = 1,
    BTN_MODE       = 2,
    BTN_POWER      = 3,
    BTN_FN1        = 4,
    BTN_FN2        = 5,
    BTN_OK         = 6,
    BTN_BACK       = 7,
} stdf_app_button_id_t;

typedef enum {
    BTN_EVT_PRESS   = 0,
    BTN_EVT_RELEASE = 1,
    BTN_EVT_HOLD    = 2,
    BTN_EVT_LONG    = 3,
} stdf_app_button_evt_t;

typedef struct {
    stdf_app_button_id_t      id;
    stdf_app_button_evt_t     event;
    uint32_t                  hold_ms;
} stdf_app_button_event_t;

typedef enum {
    TOUCH_EVT_DOWN  = 0,
    TOUCH_EVT_UP    = 1,
    TOUCH_EVT_MOVE  = 2,
} stdf_app_touch_evt_t;

typedef struct {
    stdf_app_touch_evt_t      event;
    int32_t                   x;
    int32_t                   y;
    uint32_t                  touch_id;
} stdf_app_touch_event_t;

typedef struct {
    int32_t                   delta;
    uint8_t                   direction;
} stdf_app_encoder_event_t;

typedef enum {
    REMOTE_CMD_TAKE_PHOTO = 0,
    REMOTE_CMD_RECORD_TOGGLE = 1,
    REMOTE_CMD_GIMBAL_RECENTER = 2,
    REMOTE_CMD_MODE_SWITCH = 3,
} stdf_app_remote_cmd_t;

typedef struct {
    stdf_app_remote_cmd_t     cmd;
    uint32_t                  value;
} stdf_app_remote_event_t;

typedef struct {
    char                     text[64];
    float                    confidence;
} stdf_app_voice_event_t;

#endif
