/* Network interface snapshots use inline storage for MVC copying. */
#ifndef __STDF_MVC_DATA_NET_H__
#define __STDF_MVC_DATA_NET_H__
#include <stdint.h>
#define STDF_NET_MAX_INTERFACES 16u
#define STDF_NET_LINK 1u
#define STDF_NET_IPV4 2u
#define STDF_NET_RATE 4u
#define STDF_NET_WIFI_QUALITY 8u
#define STDF_NET_WIFI_DBM 16u
#define STDF_NET_TYPE_WIRELESS 1u
#define STDF_NET_TYPE_ETHERNET 2u
#define STDF_NET_TYPE_LOOPBACK 4u
#define STDF_NET_TYPE_OTHER 8u

typedef struct stdf_mvc_net_interface_t {
    char name[64];
    char ipv4[16];
    uint32_t index;
    uint32_t valid;
    uint32_t link_up;
    uint32_t type;
    uint32_t wireless;
    double receive_bytes_per_second;
    double transmit_bytes_per_second;
    float wifi_quality;
    float wifi_signal_dbm;
} stdf_mvc_net_interface_t;

typedef struct stdf_mvc_net_interfaces_t {
    uint64_t timestamp_ms;
    uint32_t valid;
    uint32_t count;
    uint32_t truncated;
    stdf_mvc_net_interface_t interfaces[STDF_NET_MAX_INTERFACES];
} stdf_mvc_net_interfaces_t;

#define STDF_MVC_NET_DATA_LIST(X) \
    X(STDF_MVC_SUBJECT_NET_INTERFACES, stdf_mvc_net_interfaces_t)
#endif
