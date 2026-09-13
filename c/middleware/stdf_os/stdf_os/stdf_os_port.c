/* stdf_os_port - Linux/POSIX 实现（pthread + SIGEV_THREAD timer）。默认 Linux 即用。
   业务模块只调 stdf_os_port_*，不直接 include <pthread.h>/<signal.h>/<time.h>。 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "stdf_define.h"
#include "stdf_os_port.h"

/* ---------- mailbox ----------
 * 内部维护：base = capacity 个 slot_size 大小的内存块；
 *           slot_used[i] 区分 slot 是否被 alloc；ring[] 维护 ready FIFO 队列。
 * alloc: 从空闲 slot 池拿一块，返回 base + i*slot_size（用户 cast 成自己结构体写入）。
 * put:   把 alloc 出来的 slot 索引加入 ring[]（FIFO）。
 * get:   从 ring[] 头部取出 slot，返回 slot 地址（用户 cast 后读取）。
 * free:  把 slot_used[i] 置 false，slot 退回空闲池。 */

struct stdf_os_port_mailbox_t
{
    uint32_t          capacity;
    uint32_t          slot_size;
    char             *base;
    bool             *slot_used;
    uint32_t          used_count;
    uint32_t         *ring;
    uint32_t          ring_head;
    uint32_t          ring_tail;
    uint32_t          ring_count;
    pthread_mutex_t   mtx;
    pthread_cond_t    cond_get;
    pthread_cond_t    cond_alloc;
};

stdf_os_port_mailbox_t *stdf_os_port_mailbox_create(uint32_t capacity, uint32_t slot_size)
{
    if (capacity == 0 || slot_size == 0) { return NULL; }
    stdf_os_port_mailbox_t *m = (stdf_os_port_mailbox_t *)calloc(1, sizeof(*m));
    if (m == NULL) { return NULL; }
    m->base      = (char *)calloc(capacity, slot_size);
    m->slot_used = (bool  *)calloc(capacity, sizeof(bool));
    m->ring      = (uint32_t *)calloc(capacity, sizeof(uint32_t));
    if (m->base == NULL || m->slot_used == NULL || m->ring == NULL)
    {
        free(m->base); free(m->slot_used); free(m->ring); free(m);
        return NULL;
    }
    m->capacity   = capacity;
    m->slot_size  = slot_size;
    m->used_count = 0;
    m->ring_head  = 0;
    m->ring_tail  = 0;
    m->ring_count = 0;
    pthread_mutex_init(&m->mtx, NULL);
    pthread_cond_init(&m->cond_get, NULL);
    pthread_cond_init(&m->cond_alloc, NULL);
    return m;
}

void stdf_os_port_mailbox_destroy(stdf_os_port_mailbox_t *m)
{
    if (m == NULL) { return; }
    pthread_cond_destroy(&m->cond_alloc);
    pthread_cond_destroy(&m->cond_get);
    pthread_mutex_destroy(&m->mtx);
    free(m->ring);
    free(m->slot_used);
    free(m->base);
    free(m);
}

void *stdf_os_port_mailbox_alloc(stdf_os_port_mailbox_t *m, uint32_t timeout_ms)
{
    struct timespec deadline;
    bool has_deadline = false;
    if (timeout_ms != STDF_OS_PORT_WAIT_FOREVER && timeout_ms != 0)
    {
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec  += (time_t)(timeout_ms / 1000u);
        deadline.tv_nsec += (long)((timeout_ms % 1000u) * 1000000L);
        if (deadline.tv_nsec >= 1000000000L) { deadline.tv_sec++; deadline.tv_nsec -= 1000000000L; }
        has_deadline = true;
    }

    pthread_mutex_lock(&m->mtx);
    while (m->used_count >= m->capacity)
    {
        if (timeout_ms == 0)                 { pthread_mutex_unlock(&m->mtx); return NULL; }
        if (timeout_ms == STDF_OS_PORT_WAIT_FOREVER)
        {
            pthread_cond_wait(&m->cond_alloc, &m->mtx);
        }
        else if (has_deadline && pthread_cond_timedwait(&m->cond_alloc, &m->mtx, &deadline) == ETIMEDOUT)
        {
            pthread_mutex_unlock(&m->mtx);
            return NULL;
        }
    }
    for (uint32_t i = 0; i < m->capacity; i++)
    {
        if (!m->slot_used[i])
        {
            m->slot_used[i] = true;
            m->used_count++;
            void *slot = m->base + (size_t)i * m->slot_size;
            pthread_mutex_unlock(&m->mtx);
            return slot;
        }
    }
    pthread_mutex_unlock(&m->mtx);
    return NULL;
}

int stdf_os_port_mailbox_put(stdf_os_port_mailbox_t *m, void *block)
{
    pthread_mutex_lock(&m->mtx);
    uint32_t idx = (uint32_t)(((char *)block - m->base) / m->slot_size);
    m->ring[m->ring_tail] = idx;
    m->ring_tail = (m->ring_tail + 1u) % m->capacity;
    m->ring_count++;
    pthread_cond_signal(&m->cond_get);
    pthread_mutex_unlock(&m->mtx);
    return 0;
}

void *stdf_os_port_mailbox_get(stdf_os_port_mailbox_t *m, uint32_t timeout_ms)
{
    struct timespec deadline;
    bool has_deadline = false;
    if (timeout_ms != STDF_OS_PORT_WAIT_FOREVER && timeout_ms != 0)
    {
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec  += (time_t)(timeout_ms / 1000u);
        deadline.tv_nsec += (long)((timeout_ms % 1000u) * 1000000L);
        if (deadline.tv_nsec >= 1000000000L) { deadline.tv_sec++; deadline.tv_nsec -= 1000000000L; }
        has_deadline = true;
    }

    pthread_mutex_lock(&m->mtx);
    while (m->ring_count == 0)
    {
        if (timeout_ms == 0)                 { pthread_mutex_unlock(&m->mtx); return NULL; }
        if (timeout_ms == STDF_OS_PORT_WAIT_FOREVER)
        {
            pthread_cond_wait(&m->cond_get, &m->mtx);
        }
        else if (has_deadline && pthread_cond_timedwait(&m->cond_get, &m->mtx, &deadline) == ETIMEDOUT)
        {
            pthread_mutex_unlock(&m->mtx);
            return NULL;
        }
    }
    uint32_t idx = m->ring[m->ring_head];
    m->ring_head = (m->ring_head + 1u) % m->capacity;
    m->ring_count--;
    void *slot = m->base + (size_t)idx * m->slot_size;
    pthread_cond_signal(&m->cond_alloc);
    pthread_mutex_unlock(&m->mtx);
    return slot;
}

int stdf_os_port_mailbox_free(stdf_os_port_mailbox_t *m, void *block)
{
    pthread_mutex_lock(&m->mtx);
    uint32_t idx = (uint32_t)(((char *)block - m->base) / m->slot_size);
    m->slot_used[idx] = false;
    m->used_count--;
    pthread_cond_signal(&m->cond_alloc);
    pthread_mutex_unlock(&m->mtx);
    return 0;
}

/* ---------- timer ---------- */

typedef struct
{
    stdf_os_port_timer_cb_t   cb;
    void                     *arg;
    timer_t                   id;
    bool                      created;
} stdf_os_port_timer_ctx_t;

struct stdf_os_port_timer_t
{
    stdf_os_port_timer_ctx_t  *ctx;
};

static void stdf_os_port_timer_fire(union sigval sv)
{
    stdf_os_port_timer_ctx_t *ctx = (stdf_os_port_timer_ctx_t *)sv.sival_ptr;
    if (ctx != NULL && ctx->cb != NULL)
    {
        ctx->cb(ctx->arg);
    }
}

stdf_os_port_timer_t *stdf_os_port_timer_create(stdf_os_port_timer_cb_t cb, void *arg)
{
    stdf_os_port_timer_t *t = (stdf_os_port_timer_t *)calloc(1, sizeof(*t));
    if (t == NULL) { return NULL; }
    t->ctx = (stdf_os_port_timer_ctx_t *)calloc(1, sizeof(*t->ctx));
    if (t->ctx == NULL) { free(t); return NULL; }
    t->ctx->cb  = cb;
    t->ctx->arg = arg;

    struct sigevent sev;
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify          = SIGEV_THREAD;
    sev.sigev_notify_function = stdf_os_port_timer_fire;
    sev.sigev_value.sival_ptr = t->ctx;
    if (timer_create(CLOCK_MONOTONIC, &sev, &t->ctx->id) != 0)
    {
        free(t->ctx); free(t);
        return NULL;
    }
    t->ctx->created = true;
    return t;
}

int stdf_os_port_timer_start(stdf_os_port_timer_t *t, uint32_t delay_ms)
{
    struct itimerspec its;
    memset(&its, 0, sizeof(its));
    its.it_value.tv_sec  = (time_t)(delay_ms / 1000u);
    its.it_value.tv_nsec = (long)((delay_ms % 1000u) * 1000000L);
    return (timer_settime(t->ctx->id, 0, &its, NULL) == 0) ? 0 : -1;
}

int stdf_os_port_timer_stop(stdf_os_port_timer_t *t)
{
    struct itimerspec its;
    memset(&its, 0, sizeof(its));
    return (timer_settime(t->ctx->id, 0, &its, NULL) == 0) ? 0 : -1;
}

bool stdf_os_port_timer_is_running(stdf_os_port_timer_t *t)
{
    struct itimerspec its;
    memset(&its, 0, sizeof(its));
    if (timer_gettime(t->ctx->id, &its) != 0) { return false; }
    return (its.it_value.tv_sec != 0 || its.it_value.tv_nsec != 0);
}

void stdf_os_port_timer_destroy(stdf_os_port_timer_t *t)
{
    if (t == NULL) { return; }
    if (t->ctx != NULL)
    {
        if (t->ctx->created)
        {
            timer_delete(t->ctx->id);
        }
        free(t->ctx);
    }
    free(t);
}

/* ---------- mutex ---------- */

struct stdf_os_port_mutex_t
{
    pthread_mutex_t   mtx;
};

stdf_os_port_mutex_t *stdf_os_port_mutex_create(void)
{
    stdf_os_port_mutex_t *m = (stdf_os_port_mutex_t *)calloc(1, sizeof(*m));
    if (m == NULL) { return NULL; }
    pthread_mutex_init(&m->mtx, NULL);
    return m;
}

void stdf_os_port_mutex_destroy(stdf_os_port_mutex_t *m)
{
    if (m == NULL) { return; }
    pthread_mutex_destroy(&m->mtx);
    free(m);
}

int stdf_os_port_mutex_lock(stdf_os_port_mutex_t *m, uint32_t timeout_ms)
{
    if (timeout_ms == 0)             { return (pthread_mutex_trylock(&m->mtx) == 0) ? 0 : -1; }
    if (timeout_ms == STDF_OS_PORT_WAIT_FOREVER)
    {
        pthread_mutex_lock(&m->mtx);
        return 0;
    }
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec  += (time_t)(timeout_ms / 1000u);
    deadline.tv_nsec += (long)((timeout_ms % 1000u) * 1000000L);
    if (deadline.tv_nsec >= 1000000000L) { deadline.tv_sec++; deadline.tv_nsec -= 1000000000L; }
    return (pthread_mutex_timedlock(&m->mtx, &deadline) == 0) ? 0 : -1;
}

int stdf_os_port_mutex_unlock(stdf_os_port_mutex_t *m)
{
    return (pthread_mutex_unlock(&m->mtx) == 0) ? 0 : -1;
}

/* ---------- thread ---------- */

struct stdf_os_port_thread_t
{
    pthread_t         tid;
    pthread_attr_t    attr;
    bool              attr_inited;
};

int stdf_os_port_thread_create(stdf_os_port_thread_t **t,
                               stdf_os_port_thread_entry_t entry,
                               void *arg,
                               uint32_t stack_size)
{
    stdf_os_port_thread_t *th = (stdf_os_port_thread_t *)calloc(1, sizeof(*th));
    if (th == NULL) { return -1; }
    if (pthread_attr_init(&th->attr) != 0)
    {
        free(th);
        return -1;
    }
    th->attr_inited = true;
    if (stack_size > 0)
    {
        pthread_attr_setstacksize(&th->attr, stack_size);
    }
    if (pthread_create(&th->tid, &th->attr, entry, arg) != 0)
    {
        if (th->attr_inited) { pthread_attr_destroy(&th->attr); }
        free(th);
        return -1;
    }
    *t = th;
    return 0;
}

void stdf_os_port_thread_destroy(stdf_os_port_thread_t *t)
{
    if (t == NULL) { return; }
    pthread_join(t->tid, NULL);
    if (t->attr_inited) { pthread_attr_destroy(&t->attr); }
    free(t);
}

/* ---------- current ms (CLOCK_MONOTONIC) ---------- */

uint32_t stdf_os_port_get_current_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((ts.tv_sec * 1000u) + (ts.tv_nsec / 1000000u));
}
