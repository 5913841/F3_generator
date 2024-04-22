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

__thread mbuf_cache *UDP::template_udp_pkt;
__thread uint8_t UDP::pipeline;
__thread bool UDP::flood;
__thread int UDP::global_duration_time;
__thread bool UDP::global_stop;
__thread bool UDP::payload_random;
__thread uint64_t UDP::keepalive_request_interval;
__thread void (*UDP::release_socket_callback)(Socket *sk);
void udp_start_keepalive_timer(struct Socket *sk, uint64_t now_tsc);

static __thread char g_udp_data[MBUF_DATA_SIZE] = "hello THUGEN!!\n";
void udp_set_payload(char *data, int len, int new_line)
{
    int i = 0;
    int num = 'z' - 'a' + 1;
    struct timeval tv;

    if (len == 0)
    {
        data[0] = 0;
        return;
    }

    if (!UDP::payload_random)
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

void udp_set_payload(int page_size)
{
    udp_set_payload(g_udp_data, page_size, 1);
}

static inline struct rte_mbuf *udp_new_packet(struct Socket *sk)
{
    struct rte_mbuf *m = NULL;
    struct iphdr *iph = NULL;
    struct udphdr *uh = NULL;
    struct ip6_hdr *ip6h = NULL;

    UDP *udp = (UDP *)sk->l4_protocol;

    mbuf_cache *p = udp->template_udp_pkt;

    m = mbuf_cache_alloc(udp->template_udp_pkt);
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
    uh->check = csum_pseudo_ipv4(IPPROTO_UDP, sk->src_addr, sk->dst_addr, p->data.l4_len);

    return m;
}

inline struct rte_mbuf *udp_send(struct Socket *sk)
{
    struct rte_mbuf *m = NULL;

    m = udp_new_packet(sk);
    csum_offload_ip_tcpudp(m, RTE_MBUF_F_TX_UDP_CKSUM);
    if (m)
    {
        net_stats_udp_tx();
        dpdk_config_percore::cfg_send_packet(m);
    }

    return m;
}

static void udp_close(struct Socket *sk)
{
    UDP *udp = (UDP *)sk->l4_protocol;
    udp->state = TCP_CLOSE;
    UDP::release_socket_callback(sk);
}

static inline void udp_retransmit_handler(struct Socket *sk)
{
    UDP::release_socket_callback(sk);
}

static inline void udp_send_request(UDP *udp, struct Socket *sk)
{
    udp->state = TCP_SYN_SENT;
    udp->keepalive_request_num++;
    udp_send(sk);
}

inline bool udp_in_duration()
{
    if ((current_ts_msec() < (uint64_t)(UDP::global_duration_time)) && (UDP::global_stop == false))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void udp_do_keepalive(struct Socket *sk)
{
    int pipeline = UDP::pipeline;
    UDP *udp = (UDP *)sk->l4_protocol;

    if (udp_in_duration())
    {
        /* rss auto: this socket is closed by another worker */
        if (unlikely(sk->src_addr == ip4addr_t(0)))
        {
            udp_close(sk);
            return;
        }

        /* if not flood mode, let's check packet loss */
        if ((!UDP::flood) && (udp->state == TCP_SYN_RECV))
        {
            net_stats_udp_rt();
        }

        do
        {
            udp_send_request(udp, sk);
            pipeline--;
        } while (pipeline > 0);
        if (UDP::keepalive_request_interval)
        {
            udp_start_keepalive_timer(sk, time_in_config());
        }
    }
}

struct UDPKeepAliveTimerQueue : public UniqueTimerQueue
{
    UDPKeepAliveTimerQueue()
    {
        unique_queue_init(this);
        delay_tsc = UDP::keepalive_request_interval * (g_tsc_per_second / 1000);
    }
    void callback(UniqueTimer *timer) override
    {
        udp_do_keepalive(((UDP*)timer)->socket);
    }
} udpkeepalive_timer_queue;

void udp_start_keepalive_timer(struct Socket *sk, uint64_t now_tsc)
{
    UDP *udp = (UDP *)sk->l4_protocol;
    if (udp->keepalive)
    {
        (&udpkeepalive_timer_queue, &udp->timer, now_tsc);
    }
}

static void udp_client_process(Socket *sk, struct rte_mbuf *data)
{
    ether_header *eth_hdr = rte_pktmbuf_mtod(data, struct ether_header *);
    iphdr *ip_hdr = rte_pktmbuf_mtod_offset(data, struct iphdr *, sk->l2_protocol->get_hdr_len(sk, data));
    udphdr *udp_hdr = rte_pktmbuf_mtod_offset(data, struct udphdr *, sk->l2_protocol->get_hdr_len(sk, data) + sk->l3_protocol->get_hdr_len(sk, data));

    if (unlikely(sk == NULL))
    {
        net_stats_udp_drop();
        mbuf_free(data);
    }

    UDP *udp = (UDP *)sk->l4_protocol;

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
    else if ((UDP::keepalive_request_interval == 0))
    {
        udp_send_request(udp, sk);
    }

    mbuf_free(data);
    return;
}

static void udp_server_process(struct Socket *sk, struct rte_mbuf *data)
{
    ether_header *eth_hdr = rte_pktmbuf_mtod(data, struct ether_header *);
    iphdr *ip_hdr = rte_pktmbuf_mtod_offset(data, struct iphdr *, sk->l2_protocol->get_hdr_len(sk, data));
    udphdr *udp_hdr = rte_pktmbuf_mtod_offset(data, struct udphdr *, sk->l2_protocol->get_hdr_len(sk, data) + sk->l3_protocol->get_hdr_len(sk, data));

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
    int pipeline = UDP::pipeline;
    do
    {
        udp_send(sk);
        pipeline--;
    } while (pipeline > 0);
    UDP *udp = (UDP *)sk->l4_protocol;
    if (udp->keepalive)
    {
        if (UDP::keepalive_request_interval)
        {
            /* for rtt calculationn */
            udp_start_keepalive_timer(sk, time_in_config());
        }
    }
    else if (UDP::flood)
    {
        udp_close(sk);
    }

    return num;
}

char *udp_get_payload()
{
    return g_udp_data;
}

void udp_drop(__rte_unused struct work_space *ws, struct rte_mbuf *m)
{
    if (m)
    {
        net_stats_udp_drop();
        mbuf_free2(m);
    }
}
