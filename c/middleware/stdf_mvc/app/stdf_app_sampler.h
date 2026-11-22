/* Private sampling utilities; every application owns a separate worker instance. */
#ifndef __STDF_APP_SAMPLER_H__
#define __STDF_APP_SAMPLER_H__
#include "stdf_mvc_api.h"
#include "util/stdf_mvc_log.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

typedef struct stdf_app_sampler_t {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    bool running;
    bool started;
    bool mutex_initialized;
    bool condition_initialized;
    unsigned period_ms;
    void (*sample)(void);
} stdf_app_sampler_t;

#define STDF_APP_SAMPLER_INIT(period, callback) \
    { .mutex = PTHREAD_MUTEX_INITIALIZER, .condition = PTHREAD_COND_INITIALIZER, \
      .period_ms = (period), .sample = (callback), \
      .mutex_initialized = true, .condition_initialized = true }

static inline uint64_t stdf_app_now_ms(void)
{
    struct timespec timestamp;
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    return (uint64_t)timestamp.tv_sec * 1000u + (uint64_t)timestamp.tv_nsec / 1000000u;
}

static inline bool stdf_app_read_text(const char *path, char *buffer, size_t capacity)
{
    FILE *file = fopen(path, "r");
    if (!file) return false;
    bool valid = fgets(buffer, (int)capacity, file) != NULL;
    fclose(file);
    if (!valid) return false;
    size_t length = 0;
    while (buffer[length] && buffer[length] != '\n' && buffer[length] != '\r') length++;
    buffer[length] = '\0';
    return true;
}

static inline void stdf_app_publish(stdf_mvc_subject_id_t subject, const void *sample)
{
    if (stdf_mvc_api_publish(STDF_MVC_LOCAL, subject, sample, STDF_MVC_PUB_ASYNC) != 0)
        STDF_MVC_LOG_W_TAG("[SAMPLER]", "publish failed: %d", (int)subject);
}

static inline void *stdf_app_sampler_worker(void *argument)
{
    stdf_app_sampler_t *worker = argument;
    pthread_mutex_lock(&worker->mutex);
    while (worker->running) {
        pthread_mutex_unlock(&worker->mutex);
        worker->sample();
        pthread_mutex_lock(&worker->mutex);
        struct timespec deadline;
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += worker->period_ms / 1000u;
        deadline.tv_nsec += (long)(worker->period_ms % 1000u) * 1000000L;
        if (deadline.tv_nsec >= 1000000000L) {
            deadline.tv_sec++;
            deadline.tv_nsec -= 1000000000L;
        }
        int result = 0;
        while (worker->running && result == 0)
            result = pthread_cond_timedwait(&worker->condition, &worker->mutex, &deadline);
    }
    pthread_mutex_unlock(&worker->mutex);
    return NULL;
}

static inline int stdf_app_sampler_start(stdf_app_sampler_t *worker)
{
    if (worker->started) return 0;
    if (!worker->mutex_initialized) {
        int mutex_result = pthread_mutex_init(&worker->mutex, NULL);
        if (mutex_result != 0) return -mutex_result;
        worker->mutex_initialized = true;
    }
    if (!worker->condition_initialized) {
        int condition_result = pthread_cond_init(&worker->condition, NULL);
        if (condition_result != 0) return -condition_result;
        worker->condition_initialized = true;
    }
    worker->running = true;
    int result = pthread_create(&worker->thread, NULL, stdf_app_sampler_worker, worker);
    if (result != 0) { worker->running = false; return -result; }
    worker->started = true;
    return 0;
}

static inline void stdf_app_sampler_stop(stdf_app_sampler_t *worker)
{
    if (!worker->started) return;
    pthread_mutex_lock(&worker->mutex);
    worker->running = false;
    pthread_cond_signal(&worker->condition);
    pthread_mutex_unlock(&worker->mutex);
    pthread_join(worker->thread, NULL);
    worker->started = false;
    if (worker->condition_initialized) {
        pthread_cond_destroy(&worker->condition);
        worker->condition_initialized = false;
    }
    if (worker->mutex_initialized) {
        pthread_mutex_destroy(&worker->mutex);
        worker->mutex_initialized = false;
    }
}
#endif
