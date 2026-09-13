/* main - key 单/双/三击/长按 callback 演示。
   心跳和按键事件在各自后台线程跑；Ctrl+C 退出，退出前 deinit 模拟器线程。 */

#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "stdf.h"
#include "stdf_app_key.h"
#include "stdf_app_key_sim.h"
#include "stdf_define.h"

static atomic_int g_should_exit = 0;

static void on_signal(int signum)
{
    (void)signum;
    atomic_store(&g_should_exit, 1);
}

static void on_key_event(stdf_app_key_event_t event)
{
    switch (event)
    {
        case STDF_APP_KEY_EVENT_PRESS:        fprintf(stderr, "[USER] PRESS\n");        break;
        case STDF_APP_KEY_EVENT_RELEASE:      fprintf(stderr, "[USER] RELEASE\n");      break;
        case STDF_APP_KEY_EVENT_SINGLE_CLICK: fprintf(stderr, "[USER] SINGLE_CLICK\n"); break;
        case STDF_APP_KEY_EVENT_DOUBLE_CLICK: fprintf(stderr, "[USER] DOUBLE_CLICK\n"); break;
        case STDF_APP_KEY_EVENT_TRIPLE_CLICK: fprintf(stderr, "[USER] TRIPLE_CLICK\n"); break;
        case STDF_APP_KEY_EVENT_LONG_PRESS:   fprintf(stderr, "[USER] LONG_PRESS\n");   break;
    }
}

int main(void)
{
    struct sigaction sa = {0};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    stdf_init();

    stdf_app_key_register_event_callback(STDF_APP_KEY_EVENT_MASK_ALL, on_key_event);

    STDF_LOG("demo running (Ctrl+C to exit)");

    while (!atomic_load(&g_should_exit))
    {
        sleep(1);
    }

    stdf_app_key_sim_deinit();
    STDF_LOG("bye");
    return 0;
}
