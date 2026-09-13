/* main - luna 云台相机 demo 入口 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "stdf_mvc.h"
#include "stdf_app.h"

static volatile int g_running = 1;

static void on_sigint(int sig)
{
    (void)sig;
    g_running = 0;
}

static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

static void on_button_inject(void *user_data, const stdf_mvc_signal_data_t *data)
{
    (void)user_data;
    (void)data;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);

    printf("[MAIN] luna gimbal camera demo (stdf_mvc)\n");

    if (stdf_mvc_init() != 0) {
        fprintf(stderr, "[MAIN] stdf_mvc init failed\n");
        return 1;
    }
    if (stdf_app_init() != 0) {
        fprintf(stderr, "[MAIN] stdf_app init failed\n");
        return 1;
    }

    stdf_mvc_subject_subscribe(STDF_MVC_SUBJECT_BUTTON_PRESSED,
                               on_button_inject, NULL);

    printf("[MAIN] running... (Ctrl+C to exit)\n\n");

    uint64_t start_ms   = now_ms();
    uint64_t last_dump  = start_ms;
    uint32_t loop_count = 0;

    while (g_running) {
        stdf_app_tick();

        loop_count++;
        uint64_t elapsed = now_ms() - start_ms;
        if ((elapsed - (last_dump - start_ms)) >= 5000) {
            printf("\n[MAIN] === dump @ %lu ms, loop=%u ===\n",
                   (unsigned long)elapsed, (unsigned)loop_count);
            stdf_mvc_dump_subjects();
            printf("[MAIN] pool: %u/%u used\n",
                   (unsigned)stdf_mvc_get_pool_used(),
                   (unsigned)stdf_mvc_get_pool_used() +
                   (unsigned)stdf_mvc_get_pool_free());
            last_dump = now_ms();
        }

        struct timespec sleep_ts = { .tv_sec = 0, .tv_nsec = 10 * 1000 * 1000 };
        nanosleep(&sleep_ts, NULL);
    }

    printf("\n[MAIN] shutting down...\n");
    stdf_mvc_cleanup();
    return 0;
}
