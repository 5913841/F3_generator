#ifndef __UDP_H
#define __UDP_H

#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <rte_mbuf.h>
#include <rte_common.h>
#include "dpdk/dpdk_config.h"
#include "dpdk/mbuf_template.h"
#include "timer/unique_timer.h"

void udp_set_payload(int page_size);
int udp_launch(Socket *socket);
struct rte_mbuf *udp_send(Socket *socket);
char *udp_get_payload();

struct global_udp_vars {
    uint8_t pipeline;
    bool flood;
    int global_duration_time;
    bool global_stop;
    bool payload_random;
    uint64_t keepalive_request_interval;
};

struct UDP
{
public:
    Socket *socket;
    UniqueTimer timer;
    uint8_t state;
    uint16_t keepalive_request_num : 15;
    uint16_t keepalive : 1;
    uint64_t timer_tsc;

    static udphdr *decode_hdr_pre(rte_mbuf *data)
    {
        return rte_pktmbuf_mtod_offset(data, struct udphdr *, -sizeof(struct udphdr));
    }

    size_t get_hdr_len(Socket *socket, rte_mbuf *data)
    {
        return sizeof(struct udphdr);
    }


    int process(Socket *socket, rte_mbuf *data)
    {
        return 0;
    }

    int construct(Socket *socket, rte_mbuf *data)
    {
        udphdr *udp = decode_hdr_pre(data);
        udp->len = htons(sizeof(struct udphdr) + strlen(udp_get_payload()));
        data->l4_len = sizeof(struct udphdr);
        rte_pktmbuf_prepend(data, sizeof(struct udphdr));
        return sizeof(struct udphdr);
    }
};

#endif