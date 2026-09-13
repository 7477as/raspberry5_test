/* stdf_app_key_sim - mock 按键模拟器，单线程周期跑四种 pattern。 */

#include "stdf_define.h"
#include "stdf_os.h"
#include "stdf_app_key.h"
#include "stdf_app_key_sim.h"

#include <pthread.h>
#include <time.h>

#define STDF_APP_KEY_SIM_START_DELAY_MS   500u
#define STDF_APP_KEY_SIM_PRESS_HOLD_MS    80u
#define STDF_APP_KEY_SIM_RELEASE_GAP_MS   150u
#define STDF_APP_KEY_SIM_LONG_HOLD_MS     2000u
#define STDF_APP_KEY_SIM_PATTERN_GAP_MS   2000u

static volatile bool stdf_app_key_sim_running = false;
static pthread_t     stdf_app_key_sim_thread;

static void stdf_app_key_sim_sleep_ms(uint32_t ms)
{
    const uint32_t slice_ms = 50u;
    while (ms > 0u && stdf_app_key_sim_running)
    {
        uint32_t chunk = (ms > slice_ms) ? slice_ms : ms;
        struct timespec req = {
            .tv_sec  = (time_t)(chunk / 1000u),
            .tv_nsec = (long)((chunk % 1000u) * 1000000L),
        };
        nanosleep(&req, NULL);
        ms -= chunk;
    }
}

static void stdf_app_key_sim_press_pulse(uint32_t hold_ms)
{
    stdf_app_key_inject_press();
    stdf_app_key_sim_sleep_ms(hold_ms);
    stdf_app_key_inject_release();
}

static void stdf_app_key_sim_single_click(void)
{
    stdf_app_key_sim_press_pulse(STDF_APP_KEY_SIM_PRESS_HOLD_MS);
}

static void stdf_app_key_sim_double_click(void)
{
    stdf_app_key_sim_press_pulse(STDF_APP_KEY_SIM_PRESS_HOLD_MS);
    stdf_app_key_sim_sleep_ms(STDF_APP_KEY_SIM_RELEASE_GAP_MS);
    stdf_app_key_sim_press_pulse(STDF_APP_KEY_SIM_PRESS_HOLD_MS);
}

static void stdf_app_key_sim_triple_click(void)
{
    stdf_app_key_sim_press_pulse(STDF_APP_KEY_SIM_PRESS_HOLD_MS);
    stdf_app_key_sim_sleep_ms(STDF_APP_KEY_SIM_RELEASE_GAP_MS);
    stdf_app_key_sim_press_pulse(STDF_APP_KEY_SIM_PRESS_HOLD_MS);
    stdf_app_key_sim_sleep_ms(STDF_APP_KEY_SIM_RELEASE_GAP_MS);
    stdf_app_key_sim_press_pulse(STDF_APP_KEY_SIM_PRESS_HOLD_MS);
}

static void stdf_app_key_sim_long_press(void)
{
    stdf_app_key_sim_press_pulse(STDF_APP_KEY_SIM_LONG_HOLD_MS);
}

static void *stdf_app_key_sim_thread_entry(void *argument)
{
    (void)argument;
    STDF_LOG_I("simulator start");
    stdf_app_key_sim_sleep_ms(STDF_APP_KEY_SIM_START_DELAY_MS);

    while (stdf_app_key_sim_running)
    {
        STDF_LOG_I("sim: SINGLE click");
        stdf_app_key_sim_single_click();
        if (!stdf_app_key_sim_running) { break; }
        stdf_app_key_sim_sleep_ms(STDF_APP_KEY_SIM_PATTERN_GAP_MS);

        STDF_LOG_I("sim: DOUBLE click");
        stdf_app_key_sim_double_click();
        if (!stdf_app_key_sim_running) { break; }
        stdf_app_key_sim_sleep_ms(STDF_APP_KEY_SIM_PATTERN_GAP_MS);

        STDF_LOG_I("sim: TRIPLE click");
        stdf_app_key_sim_triple_click();
        if (!stdf_app_key_sim_running) { break; }
        stdf_app_key_sim_sleep_ms(STDF_APP_KEY_SIM_PATTERN_GAP_MS);

        STDF_LOG_I("sim: LONG press");
        stdf_app_key_sim_long_press();
        if (!stdf_app_key_sim_running) { break; }
        stdf_app_key_sim_sleep_ms(STDF_APP_KEY_SIM_PATTERN_GAP_MS);
    }

    STDF_LOG_I("simulator exit");
    return NULL;
}

void stdf_app_key_sim_init(void)
{
    if (stdf_app_key_sim_running) { return; }
    stdf_app_key_sim_running = true;
    if (pthread_create(&stdf_app_key_sim_thread, NULL,
                       stdf_app_key_sim_thread_entry, NULL) != 0)
    {
        STDF_LOG_E("pthread_create failed");
        stdf_app_key_sim_running = false;
    }
}

void stdf_app_key_sim_deinit(void)
{
    if (!stdf_app_key_sim_running) { return; }
    stdf_app_key_sim_running = false;
    pthread_join(stdf_app_key_sim_thread, NULL);
}
