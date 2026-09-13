#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <time.h>

#include "stdf_define.h"
#include "stdf_mvc.h"
#include "stdf_app.h"

#define DUMP_INTERVAL_MS               (5000u)
#define LOOP_PERIOD_MS                 (10u)

static volatile sig_atomic_t g_running = 1;

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

    if (stdf_mvc_init() != 0) {
        STDF_LOG_E("%s", "stdf_mvc init failed");
        return 1;
    }
    if (stdf_app_init() != 0) {
        STDF_LOG_E("%s", "stdf_app init failed");
        return 1;
    }

    STDF_LOG_I("%s", "running... (Ctrl+C to exit)");

    uint64_t last_dump = now_ms();
    while (g_running) {
        stdf_app_tick();

        uint64_t now = now_ms();
        if (now - last_dump >= DUMP_INTERVAL_MS) {
            STDF_LOG_I("=== dump @ %lu ms ===", (unsigned long)now);
            stdf_mvc_dump_subjects();
            STDF_LOG_I("pool: %u/%u used",
                       (unsigned)stdf_mvc_get_pool_used(),
                       (unsigned)(stdf_mvc_get_pool_used() + stdf_mvc_get_pool_free()));
            last_dump = now;
        }

        struct timespec sleep_ts = { .tv_sec = 0, .tv_nsec = LOOP_PERIOD_MS * 1000000u };
        nanosleep(&sleep_ts, NULL);
    }

    STDF_LOG_I("%s", "shutting down...");
    stdf_mvc_deinit();
    return 0;
}
