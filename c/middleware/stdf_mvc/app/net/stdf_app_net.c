/* Independent multi-interface Linux network telemetry publisher. */
#include "stdf_app_net.h"
#include "app/stdf_app_sampler.h"
#include "subsystem/net/stdf_mvc_data_net.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

typedef struct network_baseline_t {
    char name[64];
    unsigned index;
    ino_t inode;
    uint64_t receive;
    uint64_t transmit;
    uint64_t timestamp_ms;
    bool valid;
} network_baseline_t;

static network_baseline_t g_baselines[STDF_NET_MAX_INTERFACES];

static bool read_network_counter(const char *name, const char *field, uint64_t *value)
{
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/net/%s/%s", name, field);
    FILE *file = fopen(path, "r");
    if (!file) return false;
    bool valid = fscanf(file, "%" SCNu64, value) == 1;
    fclose(file);
    return valid;
}

static void detect_interface_type(stdf_mvc_net_interface_t *interface)
{
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/net/%s/wireless", interface->name);
    struct stat attributes;
    if (stat(path, &attributes) == 0) {
        interface->type = STDF_NET_TYPE_WIRELESS;
        interface->wireless = 1;
        return;
    }
    snprintf(path, sizeof(path), "/sys/class/net/%s/device", interface->name);
    if (stat(path, &attributes) == 0) {
        interface->type = STDF_NET_TYPE_ETHERNET;
        return;
    }
    snprintf(path, sizeof(path), "/sys/class/net/%s/bridge", interface->name);
    if (stat(path, &attributes) == 0) {
        interface->type = STDF_NET_TYPE_ETHERNET;
        return;
    }
    interface->type = STDF_NET_TYPE_OTHER;
}

static void read_network_rates(stdf_mvc_net_interface_t *interface, unsigned slot,
                               uint64_t timestamp_ms)
{
    network_baseline_t previous = g_baselines[slot];
    network_baseline_t current = { .index = interface->index, .timestamp_ms = timestamp_ms };
    snprintf(current.name, sizeof(current.name), "%s", interface->name);
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/net/%s", interface->name);
    struct stat attributes;
    current.valid = stat(path, &attributes) == 0;
    if (current.valid) current.inode = attributes.st_ino;
    current.valid = current.valid &&
        read_network_counter(interface->name, "statistics/rx_bytes", &current.receive) &&
        read_network_counter(interface->name, "statistics/tx_bytes", &current.transmit);
    if (current.valid && previous.valid && previous.index == current.index &&
        previous.inode == current.inode && strcmp(previous.name, current.name) == 0 &&
        timestamp_ms > previous.timestamp_ms && current.receive >= previous.receive &&
        current.transmit >= previous.transmit) {
        double seconds = (double)(timestamp_ms - previous.timestamp_ms) / 1000.0;
        interface->receive_bytes_per_second = (double)(current.receive - previous.receive) / seconds;
        interface->transmit_bytes_per_second = (double)(current.transmit - previous.transmit) / seconds;
        interface->valid |= STDF_NET_RATE;
    }
    g_baselines[slot] = current;
}

static bool read_iw_signal_dbm(const char *interface_name, float *signal_dbm)
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
    char *arguments[] = { "iw", "dev", (char *)interface_name, "link", NULL };
    pid_t child = -1;
    int result = setup ? setup : posix_spawnp(&child, arguments[0], &actions,
                                             NULL, arguments, environ);
    posix_spawn_file_actions_destroy(&actions);
    close(descriptors[1]);
    if (result != 0) { close(descriptors[0]); return false; }
    int flags = fcntl(descriptors[0], F_GETFL);
    bool valid = flags >= 0 && fcntl(descriptors[0], F_SETFL, flags | O_NONBLOCK) == 0;
    char output[1024];
    size_t used = 0;
    int status = 0;
    bool finished = false;
    uint64_t deadline = stdf_app_now_ms() + 1000u;
    while (valid && stdf_app_now_ms() < deadline) {
        ssize_t length = read(descriptors[0], output + used, sizeof(output) - used - 1);
        if (length > 0) used += (size_t)length;
        else if (length < 0 && errno != EAGAIN && errno != EINTR) valid = false;
        if (used == sizeof(output) - 1) { valid = false; break; }
        pid_t waited = waitpid(child, &status, WNOHANG);
        if (waited == child) { finished = true; break; }
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
    if (!valid || !finished || !WIFEXITED(status) || WEXITSTATUS(status) != 0) return false;
    const char *marker = strstr(output, "signal:");
    if (!marker) return false;
    float value;
    if (sscanf(marker, "signal: %f dBm", &value) != 1) return false;
    if (value <= -150.0f || value >= 0.0f) return false;
    *signal_dbm = value;
    return true;
}

static void read_wireless(stdf_mvc_net_interface_t *interface)
{
    if (read_iw_signal_dbm(interface->name, &interface->wifi_signal_dbm))
        interface->valid |= STDF_NET_WIFI_DBM;
    FILE *file = fopen("/proc/net/wireless", "r");
    if (!file) return;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char name[64];
        unsigned status;
        float quality, signal, noise;
        if (sscanf(line, " %63[^:]: %x %f %f %f", name, &status,
                   &quality, &signal, &noise) != 5 || strcmp(name, interface->name) != 0) continue;
        interface->wifi_quality = quality;
        interface->valid |= STDF_NET_WIFI_QUALITY;
        break;
    }
    fclose(file);
}

static int compare_interfaces(const void *left, const void *right)
{
    const stdf_mvc_net_interface_t *first = left;
    const stdf_mvc_net_interface_t *second = right;
    return strcmp(first->name, second->name);
}

static void read_ipv4_from_netlink(stdf_mvc_net_interfaces_t *sample)
{
    int socket_fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (socket_fd < 0) return;
    struct {
        struct nlmsghdr header;
        struct ifaddrmsg message;
        char attributes[512];
    } request;
    memset(&request, 0, sizeof(request));
    request.header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    request.header.nlmsg_type = RTM_GETADDR;
    request.header.nlmsg_flags = NLM_F_REQUEST;
    request.header.nlmsg_seq = 1;
    request.message.ifa_family = AF_INET;
    if (send(socket_fd, &request, request.header.nlmsg_len, 0) < 0) {
        close(socket_fd);
        return;
    }
    char buffer[8192];
    ssize_t length = recv(socket_fd, buffer, sizeof(buffer), 0);
    close(socket_fd);
    if (length <= 0) return;
    for (struct nlmsghdr *header = (struct nlmsghdr *)buffer;
         NLMSG_OK(header, (unsigned)length); header = NLMSG_NEXT(header, length)) {
        if (header->nlmsg_type != RTM_NEWADDR) continue;
        struct ifaddrmsg *message = (struct ifaddrmsg *)NLMSG_DATA(header);
        struct rtattr *attribute = IFLA_RTA(message);
        unsigned attribute_length = IFA_PAYLOAD(header);
        for (; RTA_OK(attribute, attribute_length); attribute = RTA_NEXT(attribute, attribute_length)) {
            if (attribute->rta_type != IFA_LOCAL && attribute->rta_type != IFA_ADDRESS) continue;
            if (attribute->rta_len != RTA_LENGTH(sizeof(uint32_t))) continue;
            const struct sockaddr_in *address = (const struct sockaddr_in *)RTA_DATA(attribute);
            char text[16];
            if (!inet_ntop(AF_INET, &address->sin_addr, text, sizeof(text))) continue;
            for (unsigned slot = 0; slot < sample->count; slot++) {
                stdf_mvc_net_interface_t *interface = &sample->interfaces[slot];
                if (interface->index != message->ifa_index) continue;
                snprintf(interface->ipv4, sizeof(interface->ipv4), "%s", text);
                interface->valid |= STDF_NET_IPV4;
            }
        }
    }
}

static void read_ipv4(stdf_mvc_net_interfaces_t *sample)
{
    struct ifaddrs *addresses;
    if (getifaddrs(&addresses) == 0) {
        for (struct ifaddrs *entry = addresses; entry; entry = entry->ifa_next) {
            if (!entry->ifa_addr || entry->ifa_addr->sa_family != AF_INET) continue;
            for (unsigned slot = 0; slot < sample->count; slot++) {
                stdf_mvc_net_interface_t *interface = &sample->interfaces[slot];
                if (strcmp(interface->name, entry->ifa_name) != 0) continue;
                const struct sockaddr_in *address = (const struct sockaddr_in *)entry->ifa_addr;
                if (inet_ntop(AF_INET, &address->sin_addr, interface->ipv4, sizeof(interface->ipv4)))
                    interface->valid |= STDF_NET_IPV4;
            }
        }
        freeifaddrs(addresses);
        return;
    }
    read_ipv4_from_netlink(sample);
}

static void sample_network(void)
{
    stdf_mvc_net_interfaces_t sample = { .timestamp_ms = stdf_app_now_ms() };
    DIR *directory = opendir("/sys/class/net");
    if (directory) {
        struct dirent *entry;
        while ((entry = readdir(directory)) != NULL) {
            if (entry->d_name[0] == '.' || strcmp(entry->d_name, "lo") == 0) continue;
            if (sample.count == STDF_NET_MAX_INTERFACES) { sample.truncated = 1; continue; }
            if (strlen(entry->d_name) >= sizeof(sample.interfaces[0].name)) continue;
            stdf_mvc_net_interface_t *interface = &sample.interfaces[sample.count++];
            memcpy(interface->name, entry->d_name, strlen(entry->d_name) + 1);
            interface->index = if_nametoindex(interface->name);
            detect_interface_type(interface);
        }
        closedir(directory);
        sample.valid = 1;
    }
    qsort(sample.interfaces, sample.count, sizeof(sample.interfaces[0]), compare_interfaces);
    read_ipv4(&sample);
    for (unsigned slot = 0; slot < sample.count; slot++) {
        stdf_mvc_net_interface_t *interface = &sample.interfaces[slot];
        uint64_t carrier;
        if (read_network_counter(interface->name, "carrier", &carrier)) {
            interface->link_up = carrier != 0;
            interface->valid |= STDF_NET_LINK;
        }
        read_network_rates(interface, slot, sample.timestamp_ms);
        if (interface->type == STDF_NET_TYPE_WIRELESS) read_wireless(interface);
    }
    for (unsigned slot = sample.count; slot < STDF_NET_MAX_INTERFACES; slot++)
        g_baselines[slot].valid = false;
    stdf_app_publish(STDF_MVC_SUBJECT_NET_INTERFACES, &sample);
}

static stdf_app_sampler_t g_worker = STDF_APP_SAMPLER_INIT(1000u, sample_network);

int stdf_app_net_init(void)
{
    memset(g_baselines, 0, sizeof(g_baselines));
    return stdf_app_sampler_start(&g_worker);
}

void stdf_app_net_deinit(void)
{
    stdf_app_sampler_stop(&g_worker);
}
