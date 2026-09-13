/* stdf_mvc_payload_input - 用户输入域 payload */

#ifndef __STDF_MVC_PAYLOAD_INPUT_H__
#define __STDF_MVC_PAYLOAD_INPUT_H__

#include <stdint.h>

typedef enum {
    STDF_MVC_PAYLOAD_BTN_RECORD     = 0,
    STDF_MVC_PAYLOAD_BTN_SHUTTER    = 1,
    STDF_MVC_PAYLOAD_BTN_MODE       = 2,
    STDF_MVC_PAYLOAD_BTN_POWER      = 3,
    STDF_MVC_PAYLOAD_BTN_FN1        = 4,
    STDF_MVC_PAYLOAD_BTN_FN2        = 5,
    STDF_MVC_PAYLOAD_BTN_OK         = 6,
    STDF_MVC_PAYLOAD_BTN_BACK       = 7,
} stdf_mvc_payload_button_id_t;

typedef enum {
    STDF_MVC_PAYLOAD_BTN_EVT_PRESS   = 0,
    STDF_MVC_PAYLOAD_BTN_EVT_RELEASE = 1,
    STDF_MVC_PAYLOAD_BTN_EVT_HOLD    = 2,
    STDF_MVC_PAYLOAD_BTN_EVT_LONG    = 3,
} stdf_mvc_payload_button_evt_t;

typedef struct {
    stdf_mvc_payload_button_id_t      id;
    stdf_mvc_payload_button_evt_t     event;
    uint32_t                  hold_ms;
} stdf_mvc_payload_button_event_t;

typedef enum {
    STDF_MVC_PAYLOAD_TOUCH_EVT_DOWN  = 0,
    STDF_MVC_PAYLOAD_TOUCH_EVT_UP    = 1,
    STDF_MVC_PAYLOAD_TOUCH_EVT_MOVE  = 2,
} stdf_mvc_payload_touch_evt_t;

typedef struct {
    stdf_mvc_payload_touch_evt_t      event;
    int32_t                   x;
    int32_t                   y;
    uint32_t                  touch_id;
} stdf_mvc_payload_touch_event_t;

typedef struct {
    int32_t                   delta;
    uint8_t                   direction;
} stdf_mvc_payload_encoder_event_t;

typedef enum {
    STDF_MVC_PAYLOAD_REMOTE_CMD_TAKE_PHOTO = 0,
    STDF_MVC_PAYLOAD_REMOTE_CMD_RECORD_TOGGLE = 1,
    STDF_MVC_PAYLOAD_REMOTE_CMD_GIMBAL_RECENTER = 2,
    STDF_MVC_PAYLOAD_REMOTE_CMD_MODE_SWITCH = 3,
} stdf_mvc_payload_remote_cmd_t;

typedef struct {
    stdf_mvc_payload_remote_cmd_t     cmd;
    uint32_t                  value;
} stdf_mvc_payload_remote_event_t;

typedef struct {
    char                     text[64];
    float                    confidence;
} stdf_mvc_payload_voice_event_t;

#endif
