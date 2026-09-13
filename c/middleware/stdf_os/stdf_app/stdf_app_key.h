/* stdf_app_key - 单/双/三击/长按事件检测。
   物理按键状态通过 stdf_app_key_inject_press / stdf_app_key_inject_release 注入；
   内部用 message_send 投递到 state machine handler，用 message_send_later 调度
   click/long 事件，用 message_cancel_all 在新 press 时取消先前 pending 的 click 事件。
   模拟按键（无物理按键环境）拆到 stdf_app_key_sim.*。 */

#ifndef __STDF_APP_KEY_H__
#define __STDF_APP_KEY_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    STDF_APP_KEY_EVENT_RELEASE       = 0,
    STDF_APP_KEY_EVENT_PRESS         = 1,
    STDF_APP_KEY_EVENT_SINGLE_CLICK  = 2,
    STDF_APP_KEY_EVENT_DOUBLE_CLICK  = 3,
    STDF_APP_KEY_EVENT_TRIPLE_CLICK  = 4,
    STDF_APP_KEY_EVENT_LONG_PRESS    = 5,
} stdf_app_key_event_t;

typedef uint32_t stdf_app_key_event_mask_t;

typedef void (*stdf_app_key_event_callback_t)(stdf_app_key_event_t event);

#define STDF_APP_KEY_EVENT_MASK_CLICKS  ((1u << STDF_APP_KEY_EVENT_RELEASE)       | \
                                         (1u << STDF_APP_KEY_EVENT_PRESS)         | \
                                         (1u << STDF_APP_KEY_EVENT_SINGLE_CLICK)  | \
                                         (1u << STDF_APP_KEY_EVENT_DOUBLE_CLICK)  | \
                                         (1u << STDF_APP_KEY_EVENT_TRIPLE_CLICK))
#define STDF_APP_KEY_EVENT_MASK_LONG    (1u << STDF_APP_KEY_EVENT_LONG_PRESS)
#define STDF_APP_KEY_EVENT_MASK_ALL     (STDF_APP_KEY_EVENT_MASK_CLICKS | STDF_APP_KEY_EVENT_MASK_LONG)

void stdf_app_key_init(void);

/* 注册按键事件回调；callback 在 message 线程上下文被调用。 */
void stdf_app_key_register_event_callback(stdf_app_key_event_mask_t    event_mask,
                                          stdf_app_key_event_callback_t callback);

/* 注入一次物理 press/release（可来自 GPIO ISR 或模拟器）；
   内部通过 message_send 投递到 state machine，不会阻塞调用方。 */
void stdf_app_key_inject_press(void);
void stdf_app_key_inject_release(void);

/* 返回最近一次 inject_press/inject_release 的物理状态。 */
bool stdf_app_key_is_pressed(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_APP_KEY_H__ */
