/* stdf_app_key - 单/双/三击/长按 state machine。
   物理 press/release 通过 message_send 投递到 stdf_app_key_state_changed_handler；
   click/long 事件用 message_send_later 调度，后续 press 用 message_cancel_all 取消旧的 click 事件。
   msg_id 编码：低 8 bit = 物理状态/事件；高位保留给未来 key_num 扩展。 */

#include "stdf_define.h"
#include "stdf_os.h"
#include "stdf_app_key.h"

#include <time.h>

#define STDF_APP_KEY_MULT_PRESS_TIME_MS   500u
#define STDF_APP_KEY_LONG_PRESS_TIME_MS   1500u

#define STDF_APP_KEY_PHYS_STATE_RELEASE   0u
#define STDF_APP_KEY_PHYS_STATE_PRESS     1u

typedef enum
{
    STDF_APP_KEY_IMSG_ID_CLICK_1 = 0x0101u,
    STDF_APP_KEY_IMSG_ID_CLICK_2 = 0x0102u,
    STDF_APP_KEY_IMSG_ID_CLICK_3 = 0x0103u,
    STDF_APP_KEY_IMSG_ID_LONG    = 0x0120u,
} stdf_app_key_imsg_id_t;

static struct
{
    uint8_t                       press_count;
    uint32_t                      last_press_ms;
    uint32_t                      last_release_ms;
    stdf_app_key_event_callback_t event_callback;
    stdf_app_key_event_mask_t     event_mask;
    volatile bool                 pressed;
} stdf_app_key_state;

static uint16_t stdf_app_key_event_to_msg_id(stdf_app_key_event_t event)
{
    static const uint16_t event_msg_id[] = {
        [STDF_APP_KEY_EVENT_SINGLE_CLICK] = STDF_APP_KEY_IMSG_ID_CLICK_1,
        [STDF_APP_KEY_EVENT_DOUBLE_CLICK] = STDF_APP_KEY_IMSG_ID_CLICK_2,
        [STDF_APP_KEY_EVENT_TRIPLE_CLICK] = STDF_APP_KEY_IMSG_ID_CLICK_3,
        [STDF_APP_KEY_EVENT_LONG_PRESS]   = STDF_APP_KEY_IMSG_ID_LONG,
    };
    return event_msg_id[event];
}

static uint32_t stdf_app_key_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

static void stdf_app_key_event_msg_handler(stdf_os_msg_id_t msg_id, void *payload);

static void stdf_app_key_event_schedule(stdf_app_key_event_t event, uint32_t delay_ms)
{
    if (!(stdf_app_key_state.event_mask & (1u << event))) { return; }
    uint16_t msg_id = stdf_app_key_event_to_msg_id(event);
    message_cancel_all(stdf_app_key_event_msg_handler, msg_id);
    message_send_later(stdf_app_key_event_msg_handler, msg_id, NULL, delay_ms);
}

static void stdf_app_key_event_cancel(stdf_app_key_event_t event)
{
    if (!(stdf_app_key_state.event_mask & (1u << event))) { return; }
    message_cancel_all(stdf_app_key_event_msg_handler,
                       stdf_app_key_event_to_msg_id(event));
}

static void stdf_app_key_emit_event(stdf_app_key_event_t event)
{
    if (stdf_app_key_state.event_callback == NULL)        { return; }
    if (!(stdf_app_key_state.event_mask & (1u << event))) { return; }
    stdf_app_key_state.event_callback(event);
}

static void stdf_app_key_state_changed_handler(stdf_os_msg_id_t msg_id, void *payload)
{
    (void)payload;
    uint8_t  state = msg_id & 0xFFu;
    uint32_t now   = stdf_app_key_now_ms();

    if (state == STDF_APP_KEY_PHYS_STATE_PRESS)
    {
        if (now - stdf_app_key_state.last_release_ms >= STDF_APP_KEY_MULT_PRESS_TIME_MS)
        {
            stdf_app_key_state.press_count = 1;
        }
        else
        {
            stdf_app_key_state.press_count++;
        }
        stdf_app_key_state.last_press_ms = now;

        stdf_app_key_event_cancel(STDF_APP_KEY_EVENT_SINGLE_CLICK);
        stdf_app_key_event_cancel(STDF_APP_KEY_EVENT_DOUBLE_CLICK);
        stdf_app_key_event_cancel(STDF_APP_KEY_EVENT_TRIPLE_CLICK);
        stdf_app_key_event_schedule(STDF_APP_KEY_EVENT_LONG_PRESS,
                                    STDF_APP_KEY_LONG_PRESS_TIME_MS);

        stdf_app_key_emit_event(STDF_APP_KEY_EVENT_PRESS);
    }
    else
    {
        if (now - stdf_app_key_state.last_press_ms >= STDF_APP_KEY_LONG_PRESS_TIME_MS)
        {
            stdf_app_key_state.press_count = 0;
        }
        stdf_app_key_state.last_release_ms = now;

        stdf_app_key_event_cancel(STDF_APP_KEY_EVENT_LONG_PRESS);

        switch (stdf_app_key_state.press_count)
        {
            case 1:
                stdf_app_key_event_cancel(STDF_APP_KEY_EVENT_DOUBLE_CLICK);
                stdf_app_key_event_cancel(STDF_APP_KEY_EVENT_TRIPLE_CLICK);
                stdf_app_key_event_schedule(STDF_APP_KEY_EVENT_SINGLE_CLICK,
                                            STDF_APP_KEY_MULT_PRESS_TIME_MS);
                break;
            case 2:
                stdf_app_key_event_cancel(STDF_APP_KEY_EVENT_SINGLE_CLICK);
                stdf_app_key_event_cancel(STDF_APP_KEY_EVENT_TRIPLE_CLICK);
                stdf_app_key_event_schedule(STDF_APP_KEY_EVENT_DOUBLE_CLICK,
                                            STDF_APP_KEY_MULT_PRESS_TIME_MS);
                break;
            default:
                if (stdf_app_key_state.press_count >= 3)
                {
                    stdf_app_key_event_cancel(STDF_APP_KEY_EVENT_SINGLE_CLICK);
                    stdf_app_key_event_cancel(STDF_APP_KEY_EVENT_DOUBLE_CLICK);
                    stdf_app_key_event_schedule(STDF_APP_KEY_EVENT_TRIPLE_CLICK,
                                                STDF_APP_KEY_MULT_PRESS_TIME_MS);
                }
                break;
        }

        stdf_app_key_emit_event(STDF_APP_KEY_EVENT_RELEASE);
    }
}

static void stdf_app_key_event_msg_handler(stdf_os_msg_id_t msg_id, void *payload)
{
    (void)payload;
    switch (msg_id)
    {
        case STDF_APP_KEY_IMSG_ID_CLICK_1: stdf_app_key_emit_event(STDF_APP_KEY_EVENT_SINGLE_CLICK); break;
        case STDF_APP_KEY_IMSG_ID_CLICK_2: stdf_app_key_emit_event(STDF_APP_KEY_EVENT_DOUBLE_CLICK); break;
        case STDF_APP_KEY_IMSG_ID_CLICK_3: stdf_app_key_emit_event(STDF_APP_KEY_EVENT_TRIPLE_CLICK); break;
        case STDF_APP_KEY_IMSG_ID_LONG:    stdf_app_key_emit_event(STDF_APP_KEY_EVENT_LONG_PRESS);   break;
        default:                                                                                      break;
    }
}

void stdf_app_key_init(void)
{
    stdf_app_key_state.press_count     = 0;
    stdf_app_key_state.last_press_ms   = 0;
    stdf_app_key_state.last_release_ms = 0;
    stdf_app_key_state.event_callback  = NULL;
    stdf_app_key_state.event_mask      = 0;
    stdf_app_key_state.pressed         = false;
}

void stdf_app_key_register_event_callback(stdf_app_key_event_mask_t    event_mask,
                                          stdf_app_key_event_callback_t callback)
{
    stdf_app_key_state.event_mask     = event_mask;
    stdf_app_key_state.event_callback = callback;
}

void stdf_app_key_inject_press(void)
{
    stdf_app_key_state.pressed = true;
    message_send(stdf_app_key_state_changed_handler,
                 STDF_APP_KEY_PHYS_STATE_PRESS,
                 NULL);
}

void stdf_app_key_inject_release(void)
{
    stdf_app_key_state.pressed = false;
    message_send(stdf_app_key_state_changed_handler,
                 STDF_APP_KEY_PHYS_STATE_RELEASE,
                 NULL);
}

bool stdf_app_key_is_pressed(void)
{
    return stdf_app_key_state.pressed;
}
