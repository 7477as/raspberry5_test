/* stdf_os_config - msg id 分配 / 容量参数 / 公共类型 */

#ifndef __STDF_OS_CONFIG_H__
#define __STDF_OS_CONFIG_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STDF_OS_MSG_ID_INVALID          0xFFFFu
#define STDF_OS_IMSG_ID_BASE            0x0000u
#define STDF_OS_GMSG_ID_BASE            0x8000u

#define STDF_OS_MSG_MAILBOX_MAX         30u
#define STDF_OS_MSG_THREAD_STACK_SIZE   (4u * 1024u)

#define STDF_OS_DELAY_MSG_MAX_NUM       20u

#define STDF_OS_DELAY_MSG_FOREVER       0xFFFFFFFFu
#define STDF_OS_DELAY_MSG_IMMEDIATELY   0u

typedef uint16_t stdf_os_msg_id_t;

typedef void (*stdf_os_handler_t)(stdf_os_msg_id_t msg_id, void *payload);

typedef struct
{
    stdf_os_handler_t  handler;
    stdf_os_msg_id_t   msg_id;
    void              *payload;
} stdf_os_msg_t;

#ifdef __cplusplus
}
#endif

#endif /* __STDF_OS_CONFIG_H__ */
