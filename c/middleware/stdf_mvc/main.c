/* stdf_mvc demonstration entry point. */
#include "app/stdf_app.h"
#include "app/ui/stdf_app_ui.h"
#include "mvc/stdf_mvc.h"
#include "mvc/core/stdf_mvc_core_async.h"

#include <signal.h>
#include <time.h>

static void stop_application(int signal_number)
{
    (void)signal_number;
    g_running = 0;
}

int main(void)
{
    signal(SIGINT, stop_application);
    signal(SIGTERM, stop_application);

    if (stdf_mvc_init() != 0) {
        return 1;
    }

    int result = 1;
    if (stdf_app_init() == 0) {
        const struct timespec delay = { .tv_sec = 0, .tv_nsec = 10000000L };
        while (g_running) {
            stdf_mvc_core_async_poll();
            stdf_app_ui_poll();
            nanosleep(&delay, NULL);
        }
        result = 0;
    }

    stdf_app_deinit();
    stdf_mvc_deinit();
    return result;
}
