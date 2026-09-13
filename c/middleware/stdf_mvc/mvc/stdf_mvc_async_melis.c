/* stdf_mvc_async_melis - melis 平台异步队列占位 (RTOS 主循环消费) */

#include "stdf_mvc_async.h"
#include "stdf_mvc_emit.h"
#include "stdf_mvc_port.h"
#include "stdf_mvc_config.h"

typedef struct {
    stdf_mvc_post_async_cb_t  cb;
    void                     *payload;
} stdf_mvc_async_item_t;

typedef struct {
    uint32_t              head;
    uint32_t              tail;
    uint32_t              count;
    stdf_mvc_async_item_t queue[STDF_MVC_ASYNC_QUEUE_DEPTH];
} stdf_mvc_async_ctx_t;

static stdf_mvc_async_ctx_t s_async;

int stdf_mvc_async_queue_push_melis(stdf_mvc_post_async_cb_t cb, void *payload)
{
    if (s_async.count >= STDF_MVC_ASYNC_QUEUE_DEPTH) {
        return -1;
    }
    s_async.queue[s_async.tail].cb = cb;
    s_async.queue[s_async.tail].payload = payload;
    s_async.tail = (s_async.tail + 1) % STDF_MVC_ASYNC_QUEUE_DEPTH;
    s_async.count++;
    return 0;
}

void stdf_mvc_async_subsystem_init(void)
{
    s_async.head = 0;
    s_async.tail = 0;
    s_async.count = 0;
}

void stdf_mvc_async_subsystem_deinit(void)
{
    s_async.head = 0;
    s_async.tail = 0;
    s_async.count = 0;
}

void stdf_mvc_async_melis_poll(void)
{
    while (s_async.count > 0) {
        stdf_mvc_async_item_t item = s_async.queue[s_async.head];
        s_async.head = (s_async.head + 1) % STDF_MVC_ASYNC_QUEUE_DEPTH;
        s_async.count--;
        if (item.cb) {
            item.cb(item.payload);
        }
    }
}
