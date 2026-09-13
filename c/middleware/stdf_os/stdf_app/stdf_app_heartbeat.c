/* stdf_app_heartbeat - 周期打印实现 */

#include "stdf_define.h"
#include "stdf_os.h"
#include "stdf_app_heartbeat.h"

typedef enum
{
    STDF_APP_HEARTBEAT_IMSG_ID_TICK = 0x0201u,
} stdf_app_heartbeat_imsg_id_t;

static void stdf_app_heartbeat_msg_handler(stdf_os_msg_id_t msg_id, void *payload)
{
    (void)payload;

    if (msg_id != STDF_APP_HEARTBEAT_IMSG_ID_TICK) { return; }

    message_send_later(stdf_app_heartbeat_msg_handler,
                       STDF_APP_HEARTBEAT_IMSG_ID_TICK,
                       NULL,
                       1000);
    STDF_LOG_I("heartbeat");
}

void stdf_app_heartbeat_init(void)
{
    message_send_later(stdf_app_heartbeat_msg_handler,
        STDF_APP_HEARTBEAT_IMSG_ID_TICK,
        NULL,
        0);
}
