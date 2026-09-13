/* stdf_os - OS 子系统聚合门面（只 #include mem / msg / delay_msg 子头文件，自身不重新声明原型）。
   业务模块调 stdf_os_* API 或 message_send / message_send_later 宏语义糖。
   具体原型在各子模块 .h 里；OS API 实现细节在 stdf_os_port.*（默认 Linux/POSIX）。 */

#ifndef __STDF_OS_H__
#define __STDF_OS_H__

#include "stdf_os_config.h"
#include "stdf_os_delay_msg.h"
#include "stdf_os_mem.h"
#include "stdf_os_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化 OS 子系统（mem → msg → delay_msg）。所有 stdf_os_* / message_* 调用前必须先 init。 */
void stdf_os_init(void);

/* 语义糖宏：message_send / message_send_later / message_cancel_all / message_get_count。
   message_send_later 在 delay==0 时退化为立即投递到 mailbox。 */
#define message_send(handler, id, payload)                stdf_os_msg_mailbox_put(handler, id, payload)
#define message_send_later(handler, id, payload, delay)   do {                                                  \
                                                            if ((delay) == 0u) {                               \
                                                                stdf_os_msg_mailbox_put(handler, id, payload);\
                                                            } else {                                          \
                                                                stdf_os_delay_msg_send_later(handler, id,    \
                                                                                             payload, delay);  \
                                                            }                                                 \
                                                        } while (0)
#define message_cancel_all(handler, id)                   stdf_os_delay_msg_cancel_all(handler, id)
#define message_get_count(handler, id)                    stdf_os_delay_msg_get_count(handler, id)

#ifdef __cplusplus
}
#endif

#endif /* __STDF_OS_H__ */
