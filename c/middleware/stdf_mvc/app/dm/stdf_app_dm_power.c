/* Raspberry Pi PMIC rails, throttling state and fan speed publisher. */
#include "stdf_app_dm_power.h"
#include "app/stdf_app_sampler.h"
#include "subsystem/dm/stdf_mvc_data_dm.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
#define VCGENCMD_TIMEOUT_MS 1500u
#define THROTTLE_UNDERVOLTAGE 0x1u
#define THROTTLE_UNDER_CURRENT 0x2u
#define THROTTLE_THROTTLED 0x50000u
#define THROTTLE_SOFT_TEMPERATURE 0x20000u

static bool g_failure_reported;

static bool run_vcgencmd(char *const arguments[], char *output, size_t capacity)
{
    int descriptors[2];
    if (pipe(descriptors) != 0) return false;
    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0) {
        close(descriptors[0]); close(descriptors[1]); return false;
    }
    int setup = posix_spawn_file_actions_adddup2(&actions, descriptors[1], STDOUT_FILENO);
    setup |= posix_spawn_file_actions_addclose(&actions, descriptors[0]);
    setup |= posix_spawn_file_actions_addclose(&actions, descriptors[1]);
    setup |= posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    pid_t child = -1;
    int result = setup ? setup : posix_spawnp(&child, arguments[0], &actions,
                                             NULL, arguments, environ);
    posix_spawn_file_actions_destroy(&actions);
    close(descriptors[1]);
    if (result != 0) { close(descriptors[0]); return false; }
    int flags = fcntl(descriptors[0], F_GETFL);
    bool valid = flags >= 0 && fcntl(descriptors[0], F_SETFL, flags | O_NONBLOCK) == 0;
    size_t used = 0;
    int status = 0;
    bool finished = false;
    uint64_t deadline = stdf_app_now_ms() + VCGENCMD_TIMEOUT_MS;
    while (valid && stdf_app_now_ms() < deadline) {
        ssize_t length = read(descriptors[0], output + used, capacity - used - 1);
        if (length > 0) used += (size_t)length;
        else if (length < 0 && errno != EAGAIN && errno != EINTR) valid = false;
        if (used == capacity - 1) { valid = false; break; }
        pid_t waited = waitpid(child, &status, WNOHANG);
        if (waited == child) {
            finished = true;
            while ((length = read(descriptors[0], output + used, capacity - used - 1)) > 0) {
                used += (size_t)length;
                if (used == capacity - 1) { valid = false; break; }
            }
            break;
        }
        if (waited < 0 && errno != EINTR) { valid = false; break; }
        struct timespec delay = { .tv_nsec = 10000000L };
        nanosleep(&delay, NULL);
    }
    if (!finished) {
        kill(child, SIGKILL);
        while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
    }
    close(descriptors[0]);
    output[used] = '\0';
    return valid && finished && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static bool read_throttled_state(stdf_mvc_dm_power_t *sample)
{
    char output[128];
    char *arguments[] = { "vcgencmd", "get_throttled", NULL };
    if (!run_vcgencmd(arguments, output, sizeof(output))) return false;
    unsigned long flags = strtoul(output, NULL, 0);
    sample->throttled = (flags & THROTTLE_THROTTLED) ? 1u : 0u;
    sample->undervoltage = (flags & (THROTTLE_UNDERVOLTAGE | THROTTLE_UNDER_CURRENT)) ? 1u : 0u;
    sample->state_valid = 1;
    return true;
}

static void read_fan_speed(stdf_mvc_dm_power_t *sample)
{
    DIR *directory = opendir("/sys/class/hwmon");
    if (!directory) return;
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        if (strncmp(entry->d_name, "hwmon", 5) != 0) continue;
        char path[320];
        snprintf(path, sizeof(path), "/sys/class/hwmon/%s/fan1_input", entry->d_name);
        FILE *file = fopen(path, "r");
        if (!file) continue;
        int rpm = 0;
        if (fscanf(file, "%d", &rpm) == 1 && rpm >= 0) {
            sample->fan_rpm = (float)rpm;
            sample->fan_valid = STDF_DM_POWER_FAN_RPM;
            fclose(file);
            closedir(directory);
            return;
        }
        fclose(file);
    }
    closedir(directory);
}

static void parse_pmic_line(char *line, stdf_mvc_dm_power_t *sample)
{
    char channel[64];
    char unit;
    float value;
    if (sscanf(line, " %63s %*[^=]=%f%c", channel, &value, &unit) != 3 ||
        !isfinite(value) || value < 0 || (unit != 'V' && unit != 'A')) return;
    size_t length = strlen(channel);
    if (length < 3 || channel[length - 2] != '_' || channel[length - 1] != unit) return;
    channel[length - 2] = '\0';
    if (strlen(channel) >= sizeof(sample->rails[0].name)) return;
    unsigned index = 0;
    while (index < sample->count && strcmp(sample->rails[index].name, channel) != 0) index++;
    if (index == sample->count) {
        if (sample->count == STDF_DM_POWER_MAX_RAILS) { sample->truncated = 1; return; }
        snprintf(sample->rails[index].name, sizeof(sample->rails[index].name), "%s", channel);
        sample->count++;
    }
    stdf_mvc_dm_power_rail_t *rail = &sample->rails[index];
    if (unit == 'V') { rail->voltage_v = value; rail->valid |= STDF_DM_POWER_VOLTAGE; }
    else { rail->current_a = value; rail->valid |= STDF_DM_POWER_CURRENT; }
}

static void sample_power(void)
{
    stdf_mvc_dm_power_t sample = { .timestamp_ms = stdf_app_now_ms() };
    char output[8192];
    char *arguments[] = { "vcgencmd", "pmic_read_adc", NULL };
    if (run_vcgencmd(arguments, output, sizeof(output))) {
        char *context;
        for (char *line = strtok_r(output, "\n", &context); line;
             line = strtok_r(NULL, "\n", &context)) parse_pmic_line(line, &sample);
        sample.valid = sample.count > 0;
    }
    read_throttled_state(&sample);
    read_fan_speed(&sample);
    if (!sample.valid && !g_failure_reported)
        STDF_MVC_LOG_W_TAG("[POWER]", "%s", "PMIC unavailable; check vcgencmd and /dev/vcio access");
    g_failure_reported = !sample.valid;
    stdf_app_publish(STDF_MVC_SUBJECT_DM_POWER, &sample);
}

static stdf_app_sampler_t g_worker = STDF_APP_SAMPLER_INIT(5000u, sample_power);

int stdf_app_dm_power_init(void)
{
    g_failure_reported = false;
    return stdf_app_sampler_start(&g_worker);
}

void stdf_app_dm_power_deinit(void)
{
    stdf_app_sampler_stop(&g_worker);
}
