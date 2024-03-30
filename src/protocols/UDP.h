#ifndef __UDP_H
#define __UDP_H

#include <netinet/udp.h>
#include <rte_mbuf.h>
#include <rte_common.h>
#include "dpdk/dpdk_config.h"
#include "protocols/base_protocol.h"

struct UDP : public L4_Protocol
{
public:
    UDP() : state(TCP_CLOSE), L4_Protocol() {};
    uint8_t state;
    static std::function<void(Socket *sk)> release_socket_callback;
    uint16_t keepalive_request_num : 15;
    ProtocolCode name() override { return ProtocolCode::PTC_UDP; }
    static mbuf_cache *template_udp_pkt;
    static bool pipeline;
    static bool flood;
    static int global_duration_time;
    static bool global_stop;
    static uint64_t keepalive_request_interval;
    void udp_set_payload(int page_size);
    int udp_init();
    void udp_drop(struct rte_mbuf *m);
};


#endif