/* main.c - std_mvc demo 入口 */
#define _POSIX_C_SOURCE 200809L

#include "std_mvc_log.h"

#include <signal.h>
#include <time.h>

#include "stdf_mvc_core.h"
#include "stdf_mvc_core_config.h"
#include "stdf_mvc_core_meta.h"
#include "app/std_mvc_apps.h"

#define DUMP_INTERVAL_MS   5000u
#define LOOP_PERIOD_MS     10u

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

int main(void)
{
    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);

    if (stdf_mvc_core_init() != 0) {
        return 1;
    }
    if (app_init() != 0) {
        return 1;
    }

    STD_MVC_LOG_I("%s", "running... (Ctrl+C to exit)");

    uint64_t last_dump = now_ms();
    while (g_running) {
        app_tick();

        uint64_t now = now_ms();
        if (now - last_dump >= DUMP_INTERVAL_MS) {
            STD_MVC_LOG_I("=== dump @ %lu ms ===", (unsigned long)now);
            stdf_mvc_core_dump();
            STD_MVC_LOG_I("pool: %u/%u used",
                           (unsigned)stdf_mvc_core_get_pool_used(),
                           (unsigned)STD_MVC_POOL_SIZE);
            last_dump = now;
        }

        struct timespec ts = { .tv_sec = 0, .tv_nsec = LOOP_PERIOD_MS * 1000000u };
        nanosleep(&ts, NULL);
    }

    STD_MVC_LOG_I("%s", "shutting down...");
    app_deinit();
    stdf_mvc_core_deinit();
    return 0;
}
