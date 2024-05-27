#ifndef __UDP_H
#define __UDP_H

#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <rte_mbuf.h>
#include <rte_common.h>
#include "dpdk/dpdk_config.h"
#include "dpdk/mbuf_template.h"
#include "timer/unique_timer.h"

void udp_set_payload(int page_size, int pattern_id);
int udp_launch(Socket *socket);
void udp_send(Socket *socket);
char *udp_get_payload(int pattern_id);
void udp_validate_csum(Socket *socket);

struct global_udp_vars {
    uint8_t pipeline;
    bool flood;
    int global_duration_time;
    bool global_stop;
    bool payload_random;
    bool preset;
    uint64_t keepalive_request_interval;
};

struct global_udp_templates
{
    mbuf_cache *template_udp_pkt;
};

struct UDP
{
public:
    UniqueTimer timer;
    uint8_t state;
    uint16_t keepalive_request_num : 15;
    uint16_t keepalive : 1;
    uint16_t csum_udp;

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

    int construct(Socket *socket, rte_mbuf *data);
};



#endif