#include "protocols/UDP.h"
#include "protocols/IP.h"
#include "dpdk/dpdk_config.h"
#include "dpdk/mbuf.h"
#include "dpdk/mbuf_template.h"
#include "panel/panel.h"
#include "socket/socket.h"
#include "dpdk/csum.h"
#include <sys/time.h>
#include <rte_mbuf.h>
#include "protocols/global_states.h"
const int timer_offset = offsetof(UDP, timer) + offsetof(Socket, udp);

void udp_start_keepalive_timer(struct Socket *sk, uint64_t now_tsc);


int UDP::construct(Socket *socket, rte_mbuf *data)
{
    udphdr *udp = decode_hdr_pre(data);
    udp->len = htons(sizeof(struct udphdr) + strlen(udp_get_payload(socket->pattern)));
    data->l4_len = sizeof(struct udphdr);
    rte_pktmbuf_prepend(data, sizeof(struct udphdr));
    return sizeof(struct udphdr);
}

static __thread char g_udp_data[MAX_PATTERNS][MBUF_DATA_SIZE] = {"hello THUGEN!!\n", "hello THUGEN!!\n", "hello THUGEN!!\n", "hello THUGEN!!\n"};
void udp_set_payload(char *data, int len, int new_line, int pattern_id)
{
    int i = 0;
    int num = 'z' - 'a' + 1;
    struct timeval tv;

    if (len == 0)
    {
        data[0] = 0;
        return;
    }

    if (!g_vars[pattern_id].udp_vars.payload_random)
    {
        memset(data, 'a', len);
    }
    else
    {
        gettimeofday(&tv, NULL);
        srandom(tv.tv_usec);
        for (i = 0; i < len; i++)
        {
            data[i] = 'a' + random() % num;
        }
    }

    if ((len > 1) && new_line)
    {
        data[len - 1] = '\n';
    }

    data[len] = 0;
}

void udp_set_payload(int page_size, int pattern_id)
{
    udp_set_payload(g_udp_data[pattern_id], page_size, 1, pattern_id);
}

static inline struct rte_mbuf *udp_new_packet(struct Socket *sk)
{
    struct rte_mbuf *m = NULL;
    struct iphdr *iph = NULL;
    struct udphdr *uh = NULL;
    struct ip6_hdr *ip6h = NULL;

    UDP *udp = &sk->udp;

    mbuf_cache *p = g_templates[sk->pattern].udp_templates.template_udp_pkt;

    m = mbuf_cache_alloc(p);
    if (unlikely(m == NULL))
    {
        return NULL;
    }

    iph = rte_pktmbuf_mtod_offset(m, struct iphdr *, p->data.l2_len);
    ip6h = (struct ip6_hdr *)iph;
    uh = rte_pktmbuf_mtod_offset(m, struct udphdr *, p->data.l2_len + p->data.l3_len);

    // if (!ipv6) {
    iph->id = htons(ip_id++);
    iph->saddr = htonl(sk->src_addr);
    iph->daddr = htonl(sk->dst_addr);
    // } else {
    //     ip6h->ip6_src.s6_addr32[3] = sk->laddr;
    //     ip6h->ip6_dst.s6_addr32[3] = sk->faddr;
    // }

    uh->source = htons(sk->src_port);
    uh->dest = htons(sk->dst_port);
    uh->check = sk->udp.csum_udp;

    return m;
}

void udp_send(struct Socket *sk)
{
    struct rte_mbuf *m = NULL;

    m = udp_new_packet(sk);
    csum_offload_ip_tcpudp(m, RTE_MBUF_F_TX_UDP_CKSUM);
    if (m)
    {
        net_stats_udp_tx();
        dpdk_config_percore::cfg_send_packet(m);
    }
}

static void udp_close(struct Socket *sk)
{
    UDP *udp = &sk->udp;
    udp->state = TCP_CLOSE;
}

static inline void udp_retransmit_handler(struct Socket *sk)
{
    UDP *udp = &sk->udp;
    udp->state = TCP_CLOSE;
}

static inline void udp_send_request(UDP *udp, struct Socket *sk)
{
    udp->state = TCP_SYN_SENT;
    udp->keepalive_request_num++;
    udp_send(sk);
}

static void udp_do_keepalive(struct Socket *sk)
{
    int pipeline = g_vars[sk->pattern].udp_vars.pipeline;
    UDP *udp = &sk->udp;

    /* rss auto: this socket is closed by another worker */
    if (unlikely(sk->src_addr == ip4addr_t(0)))
    {
        udp_close(sk);
        return;
    }

    /* if not flood mode, let's check packet loss */
    if ((!g_vars[sk->pattern].udp_vars.flood) && (udp->state == TCP_SYN_RECV))
    {
        net_stats_udp_rt();
    }

    do
    {
        udp_send_request(udp, sk);
        pipeline--;
    } while (pipeline > 0);
    if (g_vars[sk->pattern].udp_vars.keepalive_request_interval)
    {
        udp_start_keepalive_timer(sk, time_in_config());
    }
}

struct UDPKeepAliveTimerQueue : public UniqueTimerQueue
{
    UDPKeepAliveTimerQueue(){
        unique_queue_init(this);
    }
    void init_tsc(int pattern)
    {
        delay_tsc = g_vars[pattern].udp_vars.keepalive_request_interval;
    }
    void callback(UniqueTimer *timer) override
    {
        udp_do_keepalive((Socket *)((void*)timer - timer_offset));
    }
};

UDPKeepAliveTimerQueue udpkeepalive_timer_queue[MAX_PATTERNS];

void udp_start_keepalive_timer(struct Socket *sk, uint64_t now_tsc)
{
    UDP *udp = &sk->udp;
    if (udp->keepalive)
    {
        unique_queue_add(udpkeepalive_timer_queue+sk->pattern, &udp->timer, now_tsc);
    }
}

static void udp_client_process(Socket *sk, struct rte_mbuf *data)
{
    ether_header *eth_hdr = rte_pktmbuf_mtod(data, struct ether_header *);
    iphdr *ip_hdr = rte_pktmbuf_mtod_offset(data, struct iphdr *, sizeof(ether_header));
    udphdr *udp_hdr = rte_pktmbuf_mtod_offset(data, struct udphdr *, sizeof(ether_header)+sizeof(iphdr));

    if (unlikely(sk == NULL))
    {
        net_stats_udp_drop();
        mbuf_free(data);
    }

    UDP *udp = &sk->udp;

    if (udp->state != TCP_CLOSE)
    {
        udp->state = TCP_SYN_RECV;
    }
    else
    {
        net_stats_udp_drop();
        mbuf_free(data);
    }
    if (udp->keepalive == 0)
    {
        net_stats_rtt(udp);
        udp_close(sk);
    }
    else if ((g_vars[sk->pattern].udp_vars.keepalive_request_interval == 0))
    {
        udp_send_request(udp, sk);
    }

    mbuf_free(data);
    return;
}

static void udp_server_process(struct Socket *sk, struct rte_mbuf *data)
{
    ether_header *eth_hdr = rte_pktmbuf_mtod(data, struct ether_header *);
    iphdr *ip_hdr = rte_pktmbuf_mtod_offset(data, struct iphdr *, sizeof(ether_header));
    udphdr *udp_hdr = rte_pktmbuf_mtod_offset(data, struct udphdr *, sizeof(ether_header)+sizeof(iphdr));

    if (unlikely(sk == NULL))
    {
        net_stats_udp_drop();
        mbuf_free(data);
    }

    udp_send(sk);
    mbuf_free(data);
    return;
}

int udp_launch(Socket *sk)
{
    uint64_t i = 0;
    uint64_t num = 0;
    int pipeline = g_vars[sk->pattern].udp_vars.pipeline;
    do
    {
        udp_send(sk);
        pipeline--;
    } while (pipeline > 0);
    UDP *udp = &sk->udp;
    if (udp->keepalive)
    {
        if (g_vars[sk->pattern].udp_vars.keepalive_request_interval)
        {
            /* for rtt calculationn */
            udp_start_keepalive_timer(sk, time_in_config());
        }
    }
    else if (g_vars[sk->pattern].udp_vars.flood)
    {
        udp_close(sk);
    }

    return num;
}

char *udp_get_payload(int pattern_id)
{
    return g_udp_data[pattern_id];
}

void udp_drop(__rte_unused struct work_space *ws, struct rte_mbuf *m)
{
    if (m)
    {
        net_stats_udp_drop();
        mbuf_free2(m);
    }
}

void udp_validate_csum(Socket *socket)
{
    UDP *udp = &socket->udp;
    udp->csum_udp = csum_pseudo_ipv4(IPPROTO_UDP, socket->src_addr, socket->dst_addr, g_templates[socket->pattern].udp_templates.template_udp_pkt->data.l4_len);
}
