/* stdf_app_heartbeat - 周期打印演示。
   每隔 HEARTBEAT_PERIOD_MS 通过 message_send_later 自重链调度下一帧；
   初值用 message_send 立即投递第一条。简单展示 message_send_later + 链式重调度。 */

#ifndef __STDF_APP_HEARTBEAT_H__
#define __STDF_APP_HEARTBEAT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void stdf_app_heartbeat_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_APP_HEARTBEAT_H__ */
