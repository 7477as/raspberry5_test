/* stdf_os_msg - 异步 mailbox 投递 + 后台消费线程 */

#ifndef __STDF_OS_MSG_H__
#define __STDF_OS_MSG_H__

#include "stdf_os_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void stdf_os_msg_init(void);

/* 投递一条消息到 mailbox，handler 将在后台线程被异步调用。
   返回 0 成功，-1 mailbox 满 / handler 为 NULL。 */
int stdf_os_msg_mailbox_put(stdf_os_handler_t handler, stdf_os_msg_id_t msg_id, void *payload);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_OS_MSG_H__ */
