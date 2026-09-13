/* stdf_os_delay_msg - 单 timer + run_time 表驱动所有延迟消息，
   算法与参考样板一致：每次 add 取最小 run_time 重启 timer；
   timer fire 先处理 latest 条目，再扫描其他已过期条目 fast call，最后重启 timer。 */

#include "stdf_define.h"
#include "stdf_os_delay_msg.h"
#include "stdf_os_msg.h"
#include "stdf_os_port.h"

typedef struct
{
    bool                used;
    bool                latest;
    stdf_os_handler_t   handler;
    stdf_os_msg_id_t    msg_id;
    void               *payload;
    uint32_t            run_time;
} stdf_os_delay_msg_data_t;

static stdf_os_delay_msg_data_t stdf_os_delay_msg_data[STDF_OS_DELAY_MSG_MAX_NUM];
static stdf_os_port_timer_t    *stdf_os_delay_msg_timer   = NULL;
static stdf_os_port_mutex_t    *stdf_os_delay_msg_mutex   = NULL;

#define STDF_OS_DELAY_MSG_ENTER_CRITICAL()  (void)stdf_os_port_mutex_lock(stdf_os_delay_msg_mutex, 500)
#define STDF_OS_DELAY_MSG_EXIT_CRITICAL()   (void)stdf_os_port_mutex_unlock(stdf_os_delay_msg_mutex)

static void stdf_os_delay_msg_deinit(uint8_t index)
{
    stdf_os_delay_msg_data[index].used     = false;
    stdf_os_delay_msg_data[index].latest   = false;
    stdf_os_delay_msg_data[index].handler  = NULL;
    stdf_os_delay_msg_data[index].msg_id   = STDF_OS_MSG_ID_INVALID;
    stdf_os_delay_msg_data[index].payload  = NULL;
    stdf_os_delay_msg_data[index].run_time = STDF_OS_DELAY_MSG_FOREVER;
}

static uint8_t stdf_os_delay_msg_get_latest(void)
{
    for (uint8_t i = 0; i < STDF_OS_DELAY_MSG_MAX_NUM; i++)
    {
        if (stdf_os_delay_msg_data[i].used && stdf_os_delay_msg_data[i].latest)
        {
            return i;
        }
    }
    return STDF_OS_DELAY_MSG_MAX_NUM;
}

static void stdf_os_delay_msg_set_latest(uint8_t index, bool enable)
{
    if (index < STDF_OS_DELAY_MSG_MAX_NUM)
    {
        stdf_os_delay_msg_data[index].latest = enable;
    }
}

static void stdf_os_delay_msg_timer_start(void)
{
    uint32_t current_ms       = stdf_os_port_get_current_ms();
    uint32_t min_run_time     = STDF_OS_DELAY_MSG_FOREVER;
    uint8_t  min_run_time_idx = STDF_OS_DELAY_MSG_MAX_NUM;

    for (uint8_t i = 0; i < STDF_OS_DELAY_MSG_MAX_NUM; i++)
    {
        if (stdf_os_delay_msg_data[i].used &&
            stdf_os_delay_msg_data[i].run_time < min_run_time)
        {
            min_run_time     = stdf_os_delay_msg_data[i].run_time;
            min_run_time_idx = i;
        }
    }

    if (min_run_time != STDF_OS_DELAY_MSG_FOREVER && min_run_time_idx < STDF_OS_DELAY_MSG_MAX_NUM)
    {
        uint32_t run_time = stdf_os_delay_msg_data[min_run_time_idx].run_time;
        if (run_time <= current_ms)
        {
            run_time = current_ms + 1u;
        }

        uint8_t old_latest = stdf_os_delay_msg_get_latest();
        if (old_latest != min_run_time_idx)
        {
            stdf_os_delay_msg_set_latest(old_latest, false);
        }
        stdf_os_delay_msg_set_latest(min_run_time_idx, true);

        if (stdf_os_port_timer_is_running(stdf_os_delay_msg_timer))
        {
            stdf_os_port_timer_stop(stdf_os_delay_msg_timer);
        }
        stdf_os_port_timer_start(stdf_os_delay_msg_timer, run_time - current_ms);
    }
}

static bool stdf_os_delay_msg_timer_is_run(void)
{
    return stdf_os_port_timer_is_running(stdf_os_delay_msg_timer);
}

static void stdf_os_delay_msg_timer_stop(void)
{
    if (stdf_os_port_timer_is_running(stdf_os_delay_msg_timer))
    {
        stdf_os_port_timer_stop(stdf_os_delay_msg_timer);
    }
}

static void stdf_os_delay_msg_timer_timeout(const void *param)
{
    (void)param;
    stdf_os_handler_t   handler;
    stdf_os_msg_id_t    msg_id;
    void               *payload;

    STDF_OS_DELAY_MSG_ENTER_CRITICAL();
    uint8_t latest_index = stdf_os_delay_msg_get_latest();
    STDF_ASSERT(latest_index < STDF_OS_DELAY_MSG_MAX_NUM);
    handler = stdf_os_delay_msg_data[latest_index].handler;
    msg_id  = stdf_os_delay_msg_data[latest_index].msg_id;
    payload = stdf_os_delay_msg_data[latest_index].payload;
    STDF_LOG_D("normal call handler %p msg_id 0x%04x payload %p",
               handler, msg_id, payload);
    stdf_os_delay_msg_set_latest(latest_index, false);
    stdf_os_delay_msg_deinit(latest_index);
    STDF_OS_DELAY_MSG_EXIT_CRITICAL();
    if (handler != NULL)
    {
        stdf_os_msg_mailbox_put(handler, msg_id, payload);
    }

    uint8_t index;
    do
    {
        STDF_OS_DELAY_MSG_ENTER_CRITICAL();
        uint32_t current_ms = stdf_os_port_get_current_ms();
        index = STDF_OS_DELAY_MSG_MAX_NUM;
        handler = NULL;
        for (uint8_t i = 0; i < STDF_OS_DELAY_MSG_MAX_NUM; i++)
        {
            if (stdf_os_delay_msg_data[i].used &&
                stdf_os_delay_msg_data[i].run_time <= current_ms)
            {
                index   = i;
                handler = stdf_os_delay_msg_data[i].handler;
                msg_id  = stdf_os_delay_msg_data[i].msg_id;
                payload = stdf_os_delay_msg_data[i].payload;
                STDF_LOG_D("fast call handler %p msg_id 0x%04x payload %p",
                           handler, msg_id, payload);
                stdf_os_delay_msg_set_latest(i, false);
                stdf_os_delay_msg_deinit(i);
                break;
            }
        }
        STDF_OS_DELAY_MSG_EXIT_CRITICAL();

        if (index < STDF_OS_DELAY_MSG_MAX_NUM && handler != NULL)
        {
            stdf_os_msg_mailbox_put(handler, msg_id, payload);
        }
    }
    while (index < STDF_OS_DELAY_MSG_MAX_NUM);

    STDF_OS_DELAY_MSG_ENTER_CRITICAL();
    if (!stdf_os_delay_msg_timer_is_run())
    {
        stdf_os_delay_msg_timer_start();
    }
    STDF_OS_DELAY_MSG_EXIT_CRITICAL();
}

static void stdf_os_delay_msg_add(stdf_os_handler_t handler,
                                  stdf_os_msg_id_t msg_id,
                                  void *payload,
                                  uint32_t delay_ms)
{
    uint8_t index;
    for (index = 0; index < STDF_OS_DELAY_MSG_MAX_NUM; index++)
    {
        if (!stdf_os_delay_msg_data[index].used) { break; }
    }

    if (index < STDF_OS_DELAY_MSG_MAX_NUM)
    {
        STDF_LOG_D("success, index %u handler %p msg_id 0x%04x delay %u",
                   index, handler, msg_id, delay_ms);

        stdf_os_delay_msg_timer_stop();
        uint32_t current_ms = stdf_os_port_get_current_ms();
        stdf_os_delay_msg_data[index].used     = true;
        stdf_os_delay_msg_data[index].handler  = handler;
        stdf_os_delay_msg_data[index].msg_id   = msg_id;
        stdf_os_delay_msg_data[index].payload  = payload;
        stdf_os_delay_msg_data[index].run_time = current_ms + delay_ms;
        stdf_os_delay_msg_timer_start();
    }
    else
    {
        STDF_LOG_W("failed, full, handler %p msg_id 0x%04x delay %u",
                   handler, msg_id, delay_ms);
    }
}

static bool stdf_os_delay_msg_delate_first(stdf_os_handler_t handler, stdf_os_msg_id_t msg_id)
{
    uint32_t min_run_time     = STDF_OS_DELAY_MSG_FOREVER;
    uint8_t  min_run_time_idx = STDF_OS_DELAY_MSG_MAX_NUM;

    for (uint8_t i = 0; i < STDF_OS_DELAY_MSG_MAX_NUM; i++)
    {
        if (stdf_os_delay_msg_data[i].used &&
            stdf_os_delay_msg_data[i].handler == handler &&
            stdf_os_delay_msg_data[i].msg_id  == msg_id &&
            stdf_os_delay_msg_data[i].run_time < min_run_time)
        {
            min_run_time     = stdf_os_delay_msg_data[i].run_time;
            min_run_time_idx = i;
        }
    }

    if (min_run_time_idx < STDF_OS_DELAY_MSG_MAX_NUM)
    {
        STDF_LOG_D("success, index %u handler %p msg_id 0x%04x",
                   min_run_time_idx, handler, msg_id);

        if (stdf_os_delay_msg_get_latest() == min_run_time_idx)
        {
            stdf_os_delay_msg_timer_stop();
            stdf_os_delay_msg_deinit(min_run_time_idx);
            stdf_os_delay_msg_timer_start();
        }
        else
        {
            stdf_os_delay_msg_deinit(min_run_time_idx);
        }
        return true;
    }
    return false;
}

void stdf_os_delay_msg_send(stdf_os_handler_t handler, stdf_os_msg_id_t msg_id, void *payload)
{
    STDF_OS_DELAY_MSG_ENTER_CRITICAL();
    stdf_os_delay_msg_add(handler, msg_id, payload, STDF_OS_DELAY_MSG_IMMEDIATELY);
    STDF_OS_DELAY_MSG_EXIT_CRITICAL();
}

void stdf_os_delay_msg_send_later(stdf_os_handler_t handler,
                                  stdf_os_msg_id_t msg_id,
                                  void *payload,
                                  uint32_t delay_ms)
{
    STDF_OS_DELAY_MSG_ENTER_CRITICAL();
    stdf_os_delay_msg_add(handler, msg_id, payload, delay_ms);
    STDF_OS_DELAY_MSG_EXIT_CRITICAL();
}

uint16_t stdf_os_delay_msg_get_count(stdf_os_handler_t handler, stdf_os_msg_id_t msg_id)
{
    uint16_t count = 0;

    STDF_OS_DELAY_MSG_ENTER_CRITICAL();
    for (uint8_t i = 0; i < STDF_OS_DELAY_MSG_MAX_NUM; i++)
    {
        if (stdf_os_delay_msg_data[i].used &&
            stdf_os_delay_msg_data[i].handler == handler &&
            stdf_os_delay_msg_data[i].msg_id  == msg_id)
        {
            count++;
        }
    }
    STDF_OS_DELAY_MSG_EXIT_CRITICAL();
    return count;
}

bool stdf_os_delay_msg_cancel_first(stdf_os_handler_t handler, stdf_os_msg_id_t msg_id)
{
    bool result;
    STDF_OS_DELAY_MSG_ENTER_CRITICAL();
    result = stdf_os_delay_msg_delate_first(handler, msg_id);
    STDF_OS_DELAY_MSG_EXIT_CRITICAL();
    return result;
}

uint16_t stdf_os_delay_msg_cancel_all(stdf_os_handler_t handler, stdf_os_msg_id_t msg_id)
{
    uint16_t count = 0;
    STDF_OS_DELAY_MSG_ENTER_CRITICAL();
    while (stdf_os_delay_msg_delate_first(handler, msg_id))
    {
        count++;
    }
    STDF_OS_DELAY_MSG_EXIT_CRITICAL();
    return count;
}

void stdf_os_delay_msg_init(void)
{
    for (uint8_t i = 0; i < STDF_OS_DELAY_MSG_MAX_NUM; i++)
    {
        stdf_os_delay_msg_data[i].used     = false;
        stdf_os_delay_msg_data[i].latest   = false;
        stdf_os_delay_msg_data[i].handler  = NULL;
        stdf_os_delay_msg_data[i].msg_id   = STDF_OS_MSG_ID_INVALID;
        stdf_os_delay_msg_data[i].payload  = NULL;
        stdf_os_delay_msg_data[i].run_time = STDF_OS_DELAY_MSG_FOREVER;
    }

    stdf_os_delay_msg_timer = stdf_os_port_timer_create(stdf_os_delay_msg_timer_timeout, NULL);
    STDF_ASSERT(stdf_os_delay_msg_timer != NULL);

    stdf_os_delay_msg_mutex = stdf_os_port_mutex_create();
    STDF_ASSERT(stdf_os_delay_msg_mutex != NULL);
}
