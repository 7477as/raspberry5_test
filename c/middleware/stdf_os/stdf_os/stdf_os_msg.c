/* stdf_os_msg - mailbox（基于 stdf_os_port_mailbox）+ 后台消费线程 */

#include "stdf_define.h"
#include "stdf_os_msg.h"
#include "stdf_os_port.h"

typedef struct
{
    stdf_os_handler_t   handler;
    stdf_os_msg_id_t    msg_id;
    void               *payload;
} stdf_os_msg_mailbox_t;

static stdf_os_port_mailbox_t *stdf_os_msg_mailbox     = NULL;
static stdf_os_port_thread_t  *stdf_os_msg_thread      = NULL;
       uint8_t                 stdf_os_msg_mailbox_cnt = 0;

static void *stdf_os_msg_thread_entry(void *argument);

static void stdf_os_msg_mailbox_init(void)
{
    stdf_os_msg_mailbox = stdf_os_port_mailbox_create(STDF_OS_MSG_MAILBOX_MAX, sizeof(stdf_os_msg_mailbox_t));
    STDF_ASSERT(stdf_os_msg_mailbox != NULL);
}

int stdf_os_msg_mailbox_put(stdf_os_handler_t handler, stdf_os_msg_id_t msg_id, void *payload)
{
    STDF_ASSERT(handler != NULL);

    stdf_os_msg_mailbox_t *msg_p = (stdf_os_msg_mailbox_t *)stdf_os_port_mailbox_alloc(stdf_os_msg_mailbox, 0);
    if (msg_p == NULL)
    {
        STDF_LOG_W("mailbox alloc failed, drop msg_id=0x%04x", msg_id);
        return -1;
    }

    msg_p->handler = handler;
    msg_p->msg_id  = msg_id;
    msg_p->payload = payload;

    if (stdf_os_port_mailbox_put(stdf_os_msg_mailbox, msg_p) != 0)
    {
        STDF_LOG_W("mailbox put failed");
        return -1;
    }
    stdf_os_msg_mailbox_cnt++;
    return 0;
}

static int stdf_os_msg_mailbox_get(stdf_os_msg_mailbox_t **msg_p)
{
    *msg_p = (stdf_os_msg_mailbox_t *)stdf_os_port_mailbox_get(stdf_os_msg_mailbox, STDF_OS_PORT_WAIT_FOREVER);
    return (*msg_p != NULL) ? 0 : -1;
}

static int stdf_os_msg_mailbox_free(stdf_os_msg_mailbox_t *msg_p)
{
    int ret = stdf_os_port_mailbox_free(stdf_os_msg_mailbox, msg_p);
    if (ret == 0) { stdf_os_msg_mailbox_cnt--; }
    return ret;
}

static void stdf_os_msg_thread_init(void)
{
    int ret = stdf_os_port_thread_create(&stdf_os_msg_thread,
                                         stdf_os_msg_thread_entry,
                                         NULL,
                                         STDF_OS_MSG_THREAD_STACK_SIZE);
    STDF_ASSERT(ret == 0);
}

static void *stdf_os_msg_thread_entry(void *argument)
{
    (void)argument;
    while (1)
    {
        stdf_os_msg_mailbox_t *msg = NULL;
        if (!stdf_os_msg_mailbox_get(&msg))
        {
            if (msg->handler != NULL)
            {
                msg->handler(msg->msg_id, msg->payload);
            }
            stdf_os_msg_mailbox_free(msg);
        }
    }
    return NULL;
}

void stdf_os_msg_init(void)
{
    stdf_os_msg_mailbox_init();
    stdf_os_msg_thread_init();
}
