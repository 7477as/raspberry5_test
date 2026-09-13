/* stdf_mvc_async - 异步发射 (Linux 实现: pthread + condvar + 环形队列) */

#include "stdf_define.h"
#include "stdf_mvc_async.h"
#include "stdf_mvc_emit.h"
#include "stdf_mvc_port.h"
#include "stdf_mvc_config.h"
#include <pthread.h>
#include <string.h>

typedef struct {
    stdf_mvc_post_async_cb_t  cb;
    void                     *payload;
} stdf_mvc_async_item_t;

typedef struct {
    pthread_mutex_t       mtx;
    pthread_cond_t        cv;
    pthread_t             worker;
    int                   running;
    uint32_t              head;
    uint32_t              tail;
    uint32_t              count;
    uint64_t              drop_count;
    stdf_mvc_async_item_t queue[STDF_MVC_ASYNC_QUEUE_DEPTH];
} stdf_mvc_async_ctx_t;

static stdf_mvc_async_ctx_t s_async;

static void *async_worker_thread(void *arg)
{
    stdf_mvc_async_ctx_t *ctx = (stdf_mvc_async_ctx_t *)arg;
    while (1) {
        pthread_mutex_lock(&ctx->mtx);
        while (ctx->running && ctx->count == 0) {
            pthread_cond_wait(&ctx->cv, &ctx->mtx);
        }
        if (!ctx->running && ctx->count == 0) {
            pthread_mutex_unlock(&ctx->mtx);
            break;
        }
        stdf_mvc_async_item_t item = ctx->queue[ctx->head];
        ctx->head = (ctx->head + 1) % STDF_MVC_ASYNC_QUEUE_DEPTH;
        ctx->count--;
        pthread_mutex_unlock(&ctx->mtx);

        if (item.cb) {
            item.cb(item.payload);
        }
    }
    return NULL;
}

int stdf_mvc_async_queue_push_linux(stdf_mvc_post_async_cb_t cb, void *payload)
{
    pthread_mutex_lock(&s_async.mtx);
    if (s_async.count >= STDF_MVC_ASYNC_QUEUE_DEPTH) {
        s_async.drop_count++;
        pthread_mutex_unlock(&s_async.mtx);
        return -1;
    }
    s_async.queue[s_async.tail].cb = cb;
    s_async.queue[s_async.tail].payload = payload;
    s_async.tail = (s_async.tail + 1) % STDF_MVC_ASYNC_QUEUE_DEPTH;
    s_async.count++;
    pthread_cond_signal(&s_async.cv);
    pthread_mutex_unlock(&s_async.mtx);
    return 0;
}

void stdf_mvc_async_subsystem_init(void)
{
    pthread_mutex_init(&s_async.mtx, NULL);
    pthread_cond_init(&s_async.cv, NULL);
    memset(s_async.queue, 0, sizeof(s_async.queue));
    s_async.head = 0;
    s_async.tail = 0;
    s_async.count = 0;
    s_async.drop_count = 0;
    s_async.running = 1;
    pthread_create(&s_async.worker, NULL, async_worker_thread, &s_async);
}

void stdf_mvc_async_subsystem_deinit(void)
{
    pthread_mutex_lock(&s_async.mtx);
    s_async.running = 0;
    pthread_cond_broadcast(&s_async.cv);
    pthread_mutex_unlock(&s_async.mtx);
    pthread_join(s_async.worker, NULL);
    pthread_mutex_destroy(&s_async.mtx);
    pthread_cond_destroy(&s_async.cv);
}

typedef struct {
    stdf_mvc_subject_id_t   subject_id;
    stdf_mvc_signal_data_t  data;
} stdf_mvc_async_emit_msg_t;

static void async_emit_dispatch(void *payload)
{
    stdf_mvc_async_emit_msg_t *msg = payload;
    stdf_mvc_subject_emit(msg->subject_id, &msg->data);
    stdf_mvc_port_mem_free(msg);
}

int stdf_mvc_subject_emit_async(stdf_mvc_subject_id_t          id,
                                  const stdf_mvc_signal_data_t  *data)
{
    if (id >= STDF_MVC_SUBJECT_COUNT || data == NULL) {
        return -1;
    }

    stdf_mvc_async_emit_msg_t *msg = stdf_mvc_port_mem_alloc(sizeof(*msg));
    if (!msg) {
        STDF_LOG_E("emit_async [%u] %s: alloc failed",
                   (unsigned)id, stdf_mvc_subject_str(id));
        return -2;
    }
    msg->subject_id = id;
    msg->data = *data;

    int ret = stdf_mvc_port_post_async(async_emit_dispatch, msg);
    if (ret != 0) {
        STDF_LOG_W("emit_async [%u] %s: queue full, drop",
                   (unsigned)id, stdf_mvc_subject_str(id));
        stdf_mvc_port_mem_free(msg);
        return -3;
    }
    return 0;
}
