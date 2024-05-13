#include "protocols/TCP.h"
#include "protocols/IP.h"
#include "protocols/ETH.h"
#include "timer/unique_timer.h"
#include "protocols/HTTP.h"
#include "dpdk/csum.h"
#include "panel/panel.h"
#include <random>
#include "timer/rand.h"
#include "socket/socket_table/socket_table.h"


// __thread bool TCP::flood;
// __thread bool TCP::server;
// __thread uint8_t TCP::send_window;
// __thread mbuf_cache *TCP::template_tcp_data;
// __thread mbuf_cache *TCP::template_tcp_opt;
// __thread mbuf_cache *TCP::template_tcp_pkt;
// __thread int TCP::global_duration_time;
// __thread bool TCP::global_keepalive;
// __thread bool TCP::global_stop;
// // ms
// __thread uint64_t TCP::keepalive_request_interval;
// __thread int TCP::setted_keepalive_request_num;
// // std::function<void(Socket *sk)> TCP::release_socket_callback;
// // std::function<void(Socket *sk)> TCP::create_socket_callback;
// // std::function<bool(FiveTuples ft, Socket *sk)> TCP::checkvalid_socket_callback;
// // __thread void (*TCP::release_socket_callback)(Socket *sk);
// // __thread void (*TCP::create_socket_callback)(Socket *sk);
// // __thread bool (*TCP::checkvalid_socket_callback)(FiveTuples ft, Socket *sk);
// __thread bool TCP::global_tcp_rst;
// __thread uint8_t TCP::tos;
// __thread bool TCP::use_http;
// __thread uint16_t TCP::global_mss;
// __thread bool TCP::constructing_opt_tmeplate;

const int timer_offset = offsetof(TCP, timer) + offsetof(Socket, tcp);

__thread global_tcp_vars TCP::g_vars[MAX_PATTERNS];
int TCP::pattern_num;

thread_local SocketPointerTable* TCP::socket_table = new SocketPointerTable();

thread_local TCP *parser_tcp = new TCP();

__thread uint32_t ip_id = 0;

extern void tcp_start_retransmit_timer(Socket *sk, uint64_t now_tsc);
extern void tcp_start_timeout_timer(Socket *sk, uint64_t now_tsc);
extern void tcp_start_keepalive_timer(Socket *sk, uint64_t now_tsc);

#define tcp_seq_lt(seq0, seq1)    ((int)((seq0) - (seq1)) < 0)
#define tcp_seq_le(seq0, seq1)    ((int)((seq0) - (seq1)) <= 0)
#define tcp_seq_gt(seq0, seq1)    ((int)((seq0) - (seq1)) > 0)
#define tcp_seq_ge(seq0, seq1)    ((int)((seq0) - (seq1)) >= 0)

bool tcp_state_before_established(uint8_t state)
{
    return state == TCP_CLOSE ||
           state == TCP_LISTEN ||
           state == TCP_SYN_SENT ||
           state == TCP_SYN_RECV;
}

bool tcp_state_after_established(uint8_t state)
{
    return state == TCP_FIN_WAIT1 ||
           state == TCP_FIN_WAIT2 ||
           state == TCP_CLOSING ||
           state == TCP_CLOSE_WAIT ||
           state == TCP_LAST_ACK ||
           state == TCP_TIME_WAIT;
}

int TCP::construct(Socket *socket, rte_mbuf *data)
{
    int opt_len = 0;
    if (g_vars[socket->pattern].constructing_opt_tmeplate && g_vars[socket->pattern].global_mss != 0)
    {
        uint8_t wscale[4] = {1, 3, 3, DEFAULT_WSCALE};
        rte_pktmbuf_prepend(data, 4);
        rte_memcpy(rte_pktmbuf_mtod(data, uint8_t *), wscale, 4);
        opt_len += 4;
        tcp_opt_mss *opt_mss = rte_pktmbuf_mtod_offset(data, struct tcp_opt_mss *, -sizeof(struct tcp_opt_mss));
        opt_mss->kind = 2;
        opt_mss->len = 4;
        opt_mss->mss = htons(g_vars[socket->pattern].global_mss);
        opt_len += sizeof(struct tcp_opt_mss);
        rte_pktmbuf_prepend(data, sizeof(struct tcp_opt_mss));
    }
    tcphdr *tcp = decode_hdr_pre(data);
    if (g_vars[socket->pattern].constructing_opt_tmeplate && g_vars[socket->pattern].global_mss != 0)
    {
        tcp->res1 = socket->pattern;
    }
    tcp->source = socket->src_port;
    tcp->dest = socket->dst_port;
    tcp->seq = snd_nxt;
    tcp->ack_seq = rcv_nxt;
    tcp->doff = (sizeof(struct tcphdr) + opt_len) / 4;
    tcp->fin = 0;
    tcp->syn = 0;
    tcp->rst = 0;
    tcp->psh = 0;
    tcp->ack = 0;
    tcp->urg = 0;
    tcp->window = htons(TCP_WIN);
    tcp->check = 0;
    tcp->urg_ptr = 0;
    data->l4_len = sizeof(struct tcphdr) + opt_len;
    rte_pktmbuf_prepend(data, sizeof(struct tcphdr));
    return sizeof(struct tcphdr) + opt_len;
}

static inline void tcp_flags_rx_count(uint8_t tcp_flags)
{
    if (tcp_flags & TH_SYN)
    {
        net_stats_syn_rx();
    }

    if (tcp_flags & TH_FIN)
    {
        net_stats_fin_rx();
    }

    if (tcp_flags & TH_RST)
    {
        net_stats_rst_rx();
    }
}

static inline void tcp_flags_tx_count(uint8_t tcp_flags)
{
    if (tcp_flags & TH_SYN)
    {
        net_stats_syn_tx();
    }

    if (tcp_flags & TH_FIN)
    {
        net_stats_fin_tx();
    }

    if (tcp_flags & TH_RST)
    {
        net_stats_rst_tx();
    }
}

static struct rte_mbuf *tcp_new_packet(struct Socket *sk, uint8_t tcp_flags)
{
    // uint16_t csum_ip = 0;
    uint16_t csum_tcp = 0;
    TCP *tcp = &sk->tcp;
    uint16_t csum_ip = 0;
    uint16_t snd_seq = 0;
    struct rte_mbuf *m = NULL;
    struct iphdr *iph = NULL;
    struct tcphdr *th = NULL;
    struct mbuf_cache *p = NULL;
    if (tcp_flags & TH_SYN)
    {
        p = tcp->g_vars[sk->pattern].template_tcp_opt;
        csum_tcp = tcp->csum_tcp_opt;
        // csum_ip = tcp->csum_ip_opt;
    }
    else if (tcp_flags & TH_PUSH)
    {
        p = tcp->g_vars[sk->pattern].template_tcp_data;
        csum_tcp = tcp->csum_tcp_data;
        // csum_ip = tcp->csum_ip_data;
        snd_seq = p->data.data_len;
        if (tcp_flags & TH_URG)
        {
            tcp_flags &= ~(TH_URG | TH_PUSH);
        }
    }
    else
    {
        p = tcp->g_vars[sk->pattern].template_tcp_pkt;
        csum_tcp = tcp->csum_tcp;
        // csum_ip = tcp->csum_ip;
    }

    m = mbuf_cache_alloc(p);

    if (unlikely(m == NULL))
    {
        return NULL;
    }

    iph = rte_pktmbuf_mtod_offset(m, struct iphdr *, p->data.l2_len);
    th = rte_pktmbuf_mtod_offset(m, struct tcphdr *, p->data.l2_len + p->data.l3_len);

    if (tcp_flags & (TH_FIN | TH_SYN))
    {
        snd_seq++;
    }
    if (tcp->snd_nxt == tcp->snd_una)
    {
        tcp->snd_nxt += snd_seq;
    }

    iph->id = htons(ip_id++);
    iph->saddr = htonl(sk->src_addr);
    iph->daddr = htonl(sk->dst_addr);

    th->th_sport = htons(sk->src_port);
    th->th_dport = htons(sk->dst_port);
    th->th_flags = tcp_flags;
    /* we always send from <snd_una> */
    th->th_seq = htonl(tcp->snd_una);
    th->th_ack = htonl(tcp->rcv_nxt);
    th->th_sum = csum_tcp;

    return m;
}

static inline void tcp_send_packet(rte_mbuf *m, struct Socket *sk)
{
    csum_offload_ip_tcpudp(m, RTE_MBUF_F_TX_TCP_CKSUM);
    dpdk_config_percore::cfg_send_packet(m);
}

static inline void tcp_unvalid_close(struct Socket *sk)
{
    if(TCP::g_vars[sk->pattern].preset){
        if(TCP::g_vars[sk->pattern].server)
            return;
        else
            sk->tcp.state = TCP_CLOSE;
            TCP::socket_table->remove_socket(sk);
    }
    else
    {
        TCP::socket_table->remove_socket(sk);
        tcp_release_socket(sk);
    }
}

static inline void tcp_socket_close(struct Socket *sk)
{
    TCP *tcp = &sk->tcp;
    unique_queue_del(&tcp->timer);
    // if (tcp->state != TCP_CLOSE)
    // {
    tcp->state = TCP_CLOSE;
    /* don't clear sequences in TIME-WAIT */
    tcp_unvalid_close(sk);
    net_stats_socket_close();
#ifdef DEBUG_
    printf("Thread: %d, tcp_close_socket: %p\n", g_config_percore->lcore_id, sk);
#endif
    // }
}

static inline void tcp_socket_create(struct Socket *sk)
{
    if(!sk->tcp.g_vars[sk->pattern].preset)
    {
        if (!TCP::g_vars[sk->pattern].preset)
        {
            TCP::socket_table->insert_socket(sk);
        }
        tcp_validate_csum(sk);
    }
    tcp_validate_socket(sk);
}

static inline void tcp_process_rst(struct Socket *sk, struct rte_mbuf *m)
{
    TCP *tcp = &sk->tcp;
    if (tcp->state != TCP_CLOSE)
    {
        tcp_socket_close(sk);
    }

    mbuf_free2(m);
}

static inline bool tcp_in_duration(int pattern)
{
    if ((TCP::g_vars[pattern].global_stop == false))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static inline void tcp_send_keepalive_request(struct Socket *sk)
{
    TCP *tcp = &sk->tcp;
    if (tcp_in_duration(sk->pattern))
    {
        tcp->keepalive_request_num++;
        tcp_reply(sk, TH_ACK | TH_PUSH);
        if (unlikely((TCP::g_vars[sk->pattern].setted_keepalive_request_num > 0) && (tcp->keepalive_request_num >= TCP::g_vars[sk->pattern].setted_keepalive_request_num)))
        {
            tcp->keepalive = 0;
        }
    }
    else
    {
        tcp->state = TCP_FIN_WAIT1;
        tcp_reply(sk, TH_ACK | TH_FIN);
        tcp->keepalive = 0;
    }
}

static inline void tcp_do_keepalive(struct Socket *sk)
{
#ifdef DEBUG_
    printf("Thread: %d, tcp_do_keepalive: %p\n", g_config_percore->lcore_id, sk);
#endif
    TCP *tcp = &sk->tcp;
    if ((tcp->snd_nxt != tcp->snd_una) || (tcp->state != TCP_ESTABLISHED) || (tcp->keepalive == 0))
    {
        if (tcp->g_vars[sk->pattern].flood)
        {
            if (tcp_in_duration(sk->pattern))
            {
                tcp_reply(sk, TH_SYN);
                tcp->snd_una = tcp->snd_nxt;
                // socket_start_keepalive_timer(sk, work_space_tsc(ws));
                tcp->timer.timer_tsc = time_in_config();
                tcp_start_keepalive_timer(sk, time_in_config());
                return;
            }
            else
            {
                tcp_socket_close(sk);
            }
        }
        return;
    }

    if (tcp->g_vars[sk->pattern].server == 0)
    {
        tcp_send_keepalive_request(sk);
    }

    return;
}

struct KeepAliveTimerQueue : public UniqueTimerQueue
{
    KeepAliveTimerQueue(){
        unique_queue_init(this);
    }
    void init_tsc(int pattern)
    {
        delay_tsc = TCP::g_vars[pattern].keepalive_request_interval;
    }
    void callback(UniqueTimer *timer) override
    {
        // tcp_do_keepalive((Socket *)timer->data);
        tcp_do_keepalive((Socket *)((void*)timer - timer_offset));
    }
};

static inline void tcp_do_timeout(struct Socket *sk)
{
#ifdef DEBUG_
    printf("Thread: %d, tcp_do_timeout: %p\n", g_config_percore->lcore_id, sk);
#endif
    TCP *tcp = &sk->tcp;
    net_stats_socket_error();
    tcp_socket_close(sk);
    return;
}

struct TimeoutTimerQueue : public UniqueTimerQueue
{
    TimeoutTimerQueue(){
        unique_queue_init(this);
    }
    void init_tsc(int pattern)
    {
        delay_tsc = (RETRANSMIT_TIMEOUT * RETRANSMIT_NUM_MAX) * (g_tsc_per_second / 1000) + TCP::g_vars[pattern].keepalive_request_interval;
    }
    void callback(UniqueTimer *timer) override
    {
        // tcp_do_timeout((Socket *)timer->data);
        tcp_do_timeout((Socket *)((void*)timer - timer_offset));
    }
};

static inline void tcp_do_retransmit(struct Socket *sk)
{
#ifdef DEBUG_
    printf("Thread: %d, tcp_do_retransmit: %p\n", g_config_percore->lcore_id, sk);
#endif
    TCP *tcp = &sk->tcp;
    uint32_t snd_nxt = 0;
    uint8_t flags = 0;

    if (tcp->snd_nxt == tcp->snd_una)
    {
        tcp->retrans = 0;
        return;
    }

    /* rss auto: this socket is closed by another worker */
    if (unlikely(sk->src_addr == ip4addr_t(0)))
    {
        tcp_socket_close(sk);
        return;
    }

    tcp->retrans++;
    if (tcp->retrans < RETRANSMIT_NUM_MAX)
    {
        flags = tcp->flags;
        // SOCKET_LOG_ENABLE(sk);
        // SOCKET_LOG(sk, "retrans");
        if ((TCP::g_vars[sk->pattern].send_window) && (tcp->snd_nxt != tcp->snd_una) && (flags & TH_PUSH))
        {
            snd_nxt = tcp->snd_nxt;
            tcp->snd_nxt = tcp->snd_una;
            tcp_reply(sk, TH_PUSH | TH_ACK);
            tcp->snd_nxt = snd_nxt;
            sk->http.snd_window = 1;
            // net_stats_push_rt();
            tcp->timer.timer_tsc = time_in_config();
            tcp_start_retransmit_timer(sk, time_in_config());
        }
        else
        {
            tcp_reply(sk, flags);
        }
        if (flags & TH_SYN)
        {
            net_stats_syn_rt();
        }
        else if (flags & TH_PUSH)
        {
            net_stats_push_rt();
        }
        else if (flags & TH_FIN)
        {
            net_stats_fin_rt();
        }
        else
        {
            net_stats_ack_rt();
        }
        return;
    }
    else
    {
        // SOCKET_LOG(sk, "err-socket");
        net_stats_socket_error();
        tcp_socket_close(sk);
        return;
    }
    return;
}

struct RetransmitTimerQueue : public UniqueTimerQueue
{
    RetransmitTimerQueue(){
        unique_queue_init(this);
    }
    void init_tsc(int pattern)
    {
        delay_tsc = RETRANSMIT_TIMEOUT * (g_tsc_per_second / 1000);
    }
    void callback(UniqueTimer *timer) override
    {
        // return tcp_do_retransmit((Socket *)timer->data);
        return tcp_do_retransmit((Socket *)((void*)timer - timer_offset));
    }
};

thread_local RetransmitTimerQueue retransmit_timer_queue[MAX_PATTERNS];
thread_local KeepAliveTimerQueue keepalive_timer_queue[MAX_PATTERNS];
thread_local TimeoutTimerQueue timeout_timer_queue[MAX_PATTERNS];

inline void tcp_start_retransmit_timer(Socket *sk, uint64_t now_tsc)
{
    TCP *tcp = &sk->tcp;
    if (tcp->snd_nxt != tcp->snd_una)
    {
        unique_queue_add(retransmit_timer_queue+sk->pattern, &tcp->timer, now_tsc);
    }
}

inline void tcp_start_timeout_timer(struct Socket *sk, uint64_t now_tsc)
{
    TCP *tcp = &sk->tcp;
    unique_queue_add(timeout_timer_queue+sk->pattern, &tcp->timer, now_tsc);
}

void tcp_start_keepalive_timer(struct Socket *sk, uint64_t now_tsc)
{
    TCP *tcp = &sk->tcp;
    if (TCP::g_vars[sk->pattern].global_keepalive && tcp->keepalive && (tcp->snd_nxt == tcp->snd_una))
    {
        unique_queue_add(keepalive_timer_queue+sk->pattern, &tcp->timer, now_tsc);
    }
}

void TCP::timer_init(int pattern)
{
    keepalive_timer_queue[pattern].init_tsc(pattern);
    timeout_timer_queue[pattern].init_tsc(pattern);
    retransmit_timer_queue[pattern].init_tsc(pattern);
    TIMERS.add_queue(keepalive_timer_queue + pattern);
    TIMERS.add_queue(timeout_timer_queue + pattern);
    TIMERS.add_queue(retransmit_timer_queue + pattern);
}

__attribute__((noinline))
void tcp_reply(struct Socket *sk, uint8_t tcp_flags)
{
    TCP *tcp = &sk->tcp;
    struct rte_mbuf *m = NULL;
    uint64_t now_tsc = 0;
    now_tsc = time_in_config();

    tcp->flags = tcp_flags;
    tcp_flags_tx_count(tcp_flags);
    net_stats_tcp_tx();
    if (tcp_flags & TH_PUSH)
    {
        if (TCP::g_vars[sk->pattern].server == 0)
        {
            net_stats_tcp_req();
            net_stats_http_get();
        }
        else
        {
            if (TCP::g_vars[sk->pattern].send_window == 0)
            {
                net_stats_tcp_rsp();
                net_stats_http_2xx();
            }
        }
    }

    m = tcp_new_packet(sk, tcp_flags);
    if (m)
    {
        tcp_send_packet(m, sk);
    }

    if (tcp->state != TCP_CLOSE)
    {
        if (tcp_flags & (TH_PUSH | TH_SYN | TH_FIN))
        {
            /* for accurate PPS */
            if ((!tcp->g_vars[sk->pattern].server) && (tcp->keepalive != 0))
            {
                now_tsc = time_in_config();
            }
            if (tcp->g_vars[sk->pattern].send_window == 0)
            {
                tcp_start_retransmit_timer(sk, now_tsc);
            }
        }
        else if (tcp->g_vars[sk->pattern].server && (tcp->g_vars[sk->pattern].send_window == 0))
        {
            tcp_start_timeout_timer(sk, now_tsc);
        }
    }

    // return m;
}

static inline void tcp_rst_set_tcp(struct tcphdr *th)
{
    uint32_t seq = 0;
    uint16_t sport = 0;
    uint16_t dport = 0;

    sport = ntohs(th->th_dport);
    dport = ntohs(th->th_sport);
    seq = ntohl(th->th_seq);
    if (th->th_flags & TH_SYN)
    {
        seq++;
    }

    if (th->th_flags & TH_FIN)
    {
        seq++;
    }

    memset(th, 0, sizeof(struct tcphdr));
    th->th_sport = htons(sport);
    th->th_dport = htons(dport);
    th->th_seq = 0;
    th->th_ack = htonl(seq);
    th->th_off = 5;
    th->th_flags = TH_RST | TH_ACK;
}

static inline void tcp_rst_set_ip(struct iphdr *iph, int pattern)
{
    uint32_t saddr = 0;
    uint32_t daddr = 0;

    daddr = ntohl(iph->saddr);
    saddr = ntohl(iph->daddr);

    memset(iph, 0, sizeof(struct iphdr));
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = TCP::g_vars[pattern].tos;
    iph->tot_len = htons(40);
    iph->ttl = DEFAULT_TTL;
    iph->frag_off = IP_FLAG_DF;
    iph->protocol = IPPROTO_TCP;
    iph->saddr = htonl(saddr);
    iph->daddr = htonl(daddr);
}

static inline void tcp_reply_rst(Socket *sk, struct ether_header *eth, struct iphdr *iph, struct tcphdr *th, rte_mbuf *m, int olen)
{
    TCP *tcp = &sk->tcp;
    int nlen = 0;

    /* not support rst yet */
    // if (ws->vxlan) {
    //     net_stats_tcp_drop();
    //     mbuf_free2(m);
    //     return;
    // }
    eth_addr_swap(eth);
    if (iph->version == 4)
    {
        tcp_rst_set_ip(iph, sk->pattern);
        tcp_rst_set_tcp(th);
        th->th_sum = rte_ipv4_udptcp_cksum((const struct rte_ipv4_hdr *)iph, th);
        iph->check = rte_ipv4_cksum((const struct rte_ipv4_hdr *)iph);
        nlen = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct tcphdr);
    }
    // else {
    //     tcp_rst_set_ipv6((struct ip6_hdr *)iph);
    //     tcp_rst_set_tcp(th);
    //     th->th_sum = RTE_IPV6_UDPTCP_CKSUM(iph, th);
    //     nlen = sizeof(struct ether_header) + sizeof(struct ip6_hdr) + sizeof(struct tcphdr);
    // }

    if (olen > nlen)
    {
        rte_pktmbuf_trim(m, olen - nlen);
    }
    net_stats_rst_tx();
    tcp_send_packet(m, sk);
}

static inline uint8_t *tcp_data_get(struct iphdr *iph, struct tcphdr *th, uint16_t *data_len)
{
    struct ip6_hdr *ip6h = (struct ip6_hdr *)iph;
    uint16_t offset = 0;
    uint16_t len = 0;
    uint8_t *data = NULL;

    if (iph->version == 4)
    {
        offset = sizeof(struct iphdr) + th->th_off * 4;
        len = ntohs(iph->tot_len) - offset;
    }
    else
    {
        offset = sizeof(struct ip6_hdr) + th->th_off * 4;
        len = ntohs(ip6h->ip6_plen) - th->th_off * 4;
    }

    data = (uint8_t *)iph + offset;
    if (data_len)
    {
        *data_len = len;
    }

    return data;
}

static inline void tcp_stop_retransmit_timer(TCP *tcp)
{
    tcp->retrans = 0;
    if (tcp->snd_nxt == tcp->snd_una)
    {
        unique_queue_del(&tcp->timer);
    }
}

static inline void tcp_reply_more(struct Socket *sk)
{
    TCP *tcp = &sk->tcp;
    int i = 0;
    uint32_t snd_una = tcp->snd_una;
    uint32_t snd_max = sk->http.snd_max;
    uint32_t snd_wnd = snd_una + tcp->g_vars[sk->pattern].send_window;

    /* wait a burst finish */
    while (tcp_seq_lt(tcp->snd_nxt, snd_wnd) && tcp_seq_lt(tcp->snd_nxt, snd_max) && (i < sk->http.snd_window))
    {
        tcp->snd_una = tcp->snd_nxt;
        tcp_reply(sk, TH_PUSH | TH_ACK | TH_URG);
        i++;
    }

    tcp->snd_una = snd_una;
    if (snd_una == snd_max)
    {
        if (tcp->keepalive)
        {
            tcp_start_timeout_timer(sk, time_in_config());
        }
        else
        {
            tcp->state = TCP_FIN_WAIT1;
            tcp_reply(sk, TH_FIN | TH_ACK);
        }
    }
}

static inline bool tcp_check_sequence(struct Socket *sk, struct tcphdr *th, uint16_t data_len)
{
    TCP *tcp = &sk->tcp;
    uint32_t ack = ntohl(th->th_ack);
    uint32_t seq = ntohl(th->th_seq);
    uint32_t snd_last = tcp->snd_una;
    uint32_t snd_nxt = 0;

    if (th->th_flags & TH_FIN)
    {
        data_len++;
    }

    if ((ack == tcp->snd_nxt) && (seq == tcp->rcv_nxt))
    {
        tcp->retrans = 0;
        if (tcp->state != TCP_CLOSE)
        {
            /* skip syn  */
            tcp->snd_una = ack;
            tcp->rcv_nxt = seq + data_len;

            if (tcp->g_vars[sk->pattern].send_window == 0)
            {
                if (snd_last != ack)
                {
                    tcp_stop_retransmit_timer(tcp);
                }
            }
            else
            {
                if (sk->http.snd_window < SEND_WINDOW_MAX)
                {
                    sk->http.snd_window++;
                }
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    /* CLOSING */
    if ((tcp->state == TCP_FIN_WAIT1) && (th->th_flags & TH_FIN) && (data_len == 1) &&
        (ack == snd_last) && (seq == tcp->rcv_nxt))
    {
        tcp->rcv_nxt = seq + 1;
        tcp->state = TCP_CLOSING;
        return true;
    }

    /* stale packet */
    if (unlikely(tcp->state == TCP_CLOSE))
    {
        return false;
    }

    // SOCKET_LOG_ENABLE(sk);
    /* my data packet lost */
    if (tcp_seq_le(ack, tcp->snd_nxt))
    {
        if (tcp->g_vars[sk->pattern].send_window)
        {
            /* new data is acked */
            if ((tcp_seq_gt(ack, tcp->snd_una)))
            {
                tcp->snd_una = ack;
                if (sk->http.snd_window < SEND_WINDOW_MAX)
                {
                    sk->http.snd_window++;
                }
                tcp->retrans = 0;
                return true;
            }
            else if (ack == tcp->snd_una)
            {
                tcp->retrans++;
                net_stats_ack_dup();
                /* 3 ACK means packets loss. */
                if (tcp->retrans < 3)
                {
                    return false;
                }

                snd_nxt = tcp->snd_nxt;
                tcp->snd_nxt = ack;
                tcp_reply(sk, TH_PUSH | TH_ACK);
                tcp->snd_nxt = snd_nxt;
                tcp->retrans = 0;
                sk->http.snd_window = 1;
                return false;
            }
            else
            {
                /* stale ack */
                return false;
            }
        }
        else
        {
            /* fast retransmit : If the last transmission time is more than 1 second */
            if ((tcp->timer.timer_tsc + TSC_PER_SEC) < time_in_config())
            {
                tcp_do_retransmit(sk);
            }
        }
    }
    else if (ack == tcp->snd_nxt)
    {
        /* lost ack */
        if (tcp_seq_le(seq, tcp->rcv_nxt))
        {
            tcp_reply(sk, TH_ACK);
            net_stats_ack_rt();
        }
    }

    return false;
}

__inline__ __attribute__((always_inline))
static void tcp_server_process_syn(struct Socket *sk, struct rte_mbuf *m, struct tcphdr *th)
{
#ifdef DEBUG_
    printf("Thread: %d, tcp_server_process_syn: %p\n", g_config_percore->lcore_id, sk);
#endif
    TCP *tcp = &sk->tcp;
    /* fast recycle */
    if (tcp->state == TCP_TIME_WAIT)
    {
        tcp->state = TCP_CLOSE;
    }

    if (tcp->state == TCP_CLOSE || tcp->state == TCP_LISTEN)
    {
        // tcp->create_socket_callback(sk);
        tcp_socket_create(sk);
        net_stats_socket_open();
        tcp->state = TCP_SYN_RECV;
        tcp->retrans = 0;
        tcp->keepalive_request_num = 0;
        tcp->snd_nxt++;
        tcp->snd_una = tcp->snd_nxt;
        sk->http.snd_window = 1;
        tcp->rcv_nxt = ntohl(th->th_seq) + 1;
        tcp_reply(sk, TH_SYN | TH_ACK);
    }
    else if (tcp->state == TCP_SYN_RECV)
    {
        /* syn-ack lost, resend it */
        // SOCKET_LOG_ENABLE(sk);
        // MBUF_LOG(m, "syn-ack-lost");
        // SOCKET_LOG(sk, "syn-ack-lost");
        if ((tcp->timer.timer_tsc + MS_PER_SEC) < time_in_config())
        {
            tcp_reply(sk, TH_SYN | TH_ACK);
            net_stats_syn_rt();
        }
    }
    else
    {
        // SOCKET_LOG_ENABLE(sk);
        // MBUF_LOG(m, "drop-syn");
        // SOCKET_LOG(sk, "drop-bad-syn");
        tcp_unvalid_close(sk);
        net_stats_tcp_drop();
    }

    mbuf_free2(m);
}

__inline__ __attribute__((always_inline))
static void tcp_client_process_syn_ack(struct Socket *sk,
                                       struct rte_mbuf *m, struct iphdr *iph, struct tcphdr *th)
{
    TCP *tcp = &sk->tcp;
    uint32_t ack = ntohl(th->th_ack);
        uint32_t seq = htonl(th->th_seq);

    if (tcp->state == TCP_SYN_SENT)
    {
        if (ack != tcp->snd_nxt)
        {
            // SOCKET_LOG_ENABLE(sk);
            // MBUF_LOG(m, "drop-syn-ack1");
            // SOCKET_LOG(sk, "drop-syn-ack1");
            net_stats_tcp_drop();
            // mbuf_free2(m);
            goto out;
            return;
        }

        net_stats_rtt(tcp);
        tcp->rcv_nxt = seq + 1;
        tcp->snd_una = ack;
        tcp->state = TCP_ESTABLISHED;
        tcp_reply(sk, TH_ACK | TH_PUSH);
    }
    else if (tcp->state == TCP_ESTABLISHED)
    {
        /* ack lost */
        net_stats_pkt_lost();
        if ((ack == tcp->snd_nxt) && ((seq + 1) == tcp->rcv_nxt))
        {
            tcp_reply(sk, TH_ACK);
            net_stats_ack_rt();
        }
    }
    else
    {
        // SOCKET_LOG_ENABLE(sk);
        // MBUF_LOG(m, "drop-syn-ack2");
        // SOCKET_LOG(sk, "drop-syn-ack-bad2");
        net_stats_tcp_drop();
    }
out:
    mbuf_free2(m);
}

static inline uint8_t tcp_process_fin(struct Socket *sk, uint8_t rx_flags, uint8_t tx_flags)
{
#ifdef DEBUG_
    printf("Thread: %d, tcp_process_fin: %p\n", g_config_percore->lcore_id, sk);
#endif
    TCP *tcp = &sk->tcp;
    uint8_t flags = 0;

    switch (tcp->state)
    {
    case TCP_ESTABLISHED:
        if (rx_flags & TH_FIN)
        {
            tcp->state = TCP_LAST_ACK;
            flags = TH_FIN | TH_ACK;
        }
        else if (tx_flags & TH_FIN)
        {
            tcp->state = TCP_FIN_WAIT1;
            flags = TH_FIN | TH_ACK;
        }
        break;
    case TCP_FIN_WAIT1:
        if (rx_flags & TH_FIN)
        {
            flags = TH_ACK;
            /* enter TIME WAIT */
            // tcp_socket_close(tcp, sk);
            sk->tcp.state = TCP_CLOSE;
        }
        else
        {
            /* wait FIN */
            tcp->state = TCP_FIN_WAIT2;
        }
        break;
    case TCP_CLOSING:
        /* In order to prevent the loss of fin, we make up a FIN, which has no cost */
        tcp->state = TCP_LAST_ACK;
        flags = TH_FIN | TH_ACK;
        break;
    case TCP_LAST_ACK:
        // tcp_socket_close(tcp, sk);
        sk->tcp.state = TCP_CLOSE;
        break;
    case TCP_FIN_WAIT2:
        /* FIN is here */
        if (rx_flags & TH_FIN)
        {
            flags = TH_ACK;
            /* enter TIME WAIT */
            // tcp_socket_close(tcp, sk);
            sk->tcp.state = TCP_CLOSE;
        }
    case TCP_TIME_WAIT:
    default:
        break;
    }

    return tx_flags | flags;
}

__inline__ __attribute__((always_inline))
static void tcp_server_process_data(struct Socket *sk, struct rte_mbuf *m,
                                           struct iphdr *iph, struct tcphdr *th)
{
#ifdef DEBUG_
    printf("Thread: %d, tcp_server_process_data: %p\n", g_config_percore->lcore_id, sk);
#endif
    TCP *tcp = &sk->tcp;
    uint8_t *data = NULL;
    uint8_t tx_flags = 0;
    uint8_t rx_flags = th->th_flags;
    uint16_t data_len = 0;

    if (tcp->state == TCP_CLOSE)
    {
        tcp_unvalid_close(sk);
        net_stats_tcp_drop();
        // mbuf_free2(m);
        goto out;
    }

    data = tcp_data_get(iph, th, &data_len);
    if (tcp_check_sequence(sk, th, data_len) == false)
    {
        // SOCKET_LOG_ENABLE(sk);
        MBUF_LOG(m, "drop-bad-seq");
        // SOCKET_LOG(sk, "drop-bad-seq");
        net_stats_tcp_drop();
        // mbuf_free2(m);
        goto out;
    }

    if (tcp_state_before_established(tcp->state))
    {
        tcp->state = TCP_ESTABLISHED;
    }

    if (tcp->state == TCP_ESTABLISHED)
    {
        if (data_len)
        {
            http_parse_request(data, data_len);
            if ((TCP::g_vars[sk->pattern].send_window) && ((rx_flags & TH_FIN) == 0))
            {
                socket_init_http_server(&sk->http, tcp, sk->pattern);
                net_stats_tcp_rsp();
                net_stats_http_2xx();
                if (tcp->keepalive_request_num)
                {
                    tcp_reply_more(sk);
                }
                else
                {
                    /* slow start */
                    tcp_reply(sk, TH_PUSH | TH_ACK);
                    tcp->keepalive_request_num = 1;
                }
                tcp_start_retransmit_timer(sk, time_in_config());
                // mbuf_free2(m);
                goto out;
                // return;
            }
            else
            {
                tx_flags |= TH_PUSH | TH_ACK;
                if (tcp->keepalive == 0)
                {
                    tx_flags |= TH_FIN;
                }
            }
        }
        else if ((tcp->g_vars[sk->pattern].send_window) && ((rx_flags & TH_FIN) == 0))
        {
            tcp_reply_more(sk);
            // mbuf_free2(m);
            goto out;
            // return;
        }
    }

    if (tcp_state_after_established(tcp->state) || ((rx_flags | tx_flags) & TH_FIN))
    {
        tx_flags = tcp_process_fin(sk, rx_flags, tx_flags);
    }

    if (tx_flags != 0)
    {
        tcp_reply(sk, tx_flags);
    }
    else if (tcp->state != TCP_CLOSE)
    {
        tcp_start_timeout_timer(sk, time_in_config());
    }

    if (sk->tcp.state == TCP_CLOSE)
    {
        tcp_socket_close(sk);
    }
out:
    mbuf_free2(m);
}

__inline__ __attribute__((always_inline))
static void tcp_server_process(Socket *sk, ether_header *eth_hdr, iphdr *ip_hdr, tcphdr *tcp_hdr, rte_mbuf *data, int olen)
{
    uint8_t flags = tcp_hdr->th_flags;
    tcp_flags_rx_count(flags);

    if (((flags & (TH_SYN | TH_RST)) == 0) && (flags & TH_ACK))
    {
        tcp_server_process_data(sk, data, ip_hdr, tcp_hdr);
    }
    else if (flags == TH_SYN)
    {
        tcp_server_process_syn(sk, data, tcp_hdr);
    }
    else if (flags & TH_RST)
    {
        tcp_process_rst(sk, data);
    }
    else
    {
        // MBUF_LOG(m, "drop-bad-flags");
        // SOCKET_LOG(sk, "drop-bad-flags");
        net_stats_tcp_drop();
        mbuf_free2(data);
    }
}

__inline__ __attribute__((always_inline))
static void tcp_client_process_data(struct Socket *sk, struct rte_mbuf *m,
                                           struct iphdr *iph, struct tcphdr *th)
{
    TCP *tcp = &sk->tcp;
    uint8_t *data = NULL;
    uint8_t tx_flags = 0;
    uint8_t rx_flags = th->th_flags;
    uint16_t data_len = 0;

    data = tcp_data_get(iph, th, &data_len);
    if (tcp_check_sequence(sk, th, data_len) == false)
    {
        // SOCKET_LOG_ENABLE(sk);
        // MBUF_LOG(m, "drop-bad-seq");
        // SOCKET_LOG(sk, "drop-bad-seq");
        net_stats_tcp_drop();
        mbuf_free2(m);
        return;
    }

    if (tcp->state == TCP_ESTABLISHED)
    {
        if (data_len)
        {
            // #ifdef HTTP_PARSE
            if (tcp->g_vars[sk->pattern].use_http)
            {
                tx_flags = http_client_process_data(sk, rx_flags, data, data_len);
            }
            else
            // #endif
            {
                tx_flags |= TH_ACK;
                http_parse_response(data, data_len);
                if (tcp->keepalive == 0)
                {
                    tx_flags |= TH_FIN;
                }
                else
                {
                    if (tcp->g_vars[sk->pattern].keepalive_request_interval)
                    {
                        tcp_start_keepalive_timer(sk, tcp->timer.timer_tsc);
                    }
                    else
                    {
                        tcp_send_keepalive_request(sk);
                    }
                }
            }
        }
    }

    if (tcp_state_after_established(tcp->state) || ((rx_flags | tx_flags) & TH_FIN))
    {
        tx_flags = tcp_process_fin(sk, rx_flags, tx_flags);
    }

    if (tx_flags != 0)
    {
        /* delay ack */
        if ((rx_flags & TH_FIN) || (tx_flags != TH_ACK) || (tcp->keepalive == 0) || (tcp->keepalive && (tcp->g_vars[sk->pattern].keepalive_request_interval >= RETRANSMIT_TIMEOUT)))
        {
            tcp_reply(sk, tx_flags);
        }
    }

    if (sk->tcp.state == TCP_CLOSE)
    {
        tcp_socket_close(sk);
    }

    mbuf_free2(m);
}

__inline__ __attribute__((always_inline))
static void tcp_client_process(Socket *sk, ether_header *eth_hdr, iphdr *ip_hdr, tcphdr *tcp_hdr, rte_mbuf *data, int olen)
{
    uint8_t flags = tcp_hdr->th_flags;
    tcp_flags_rx_count(flags);

    if (((flags & (TH_SYN | TH_RST)) == 0) && (flags & TH_ACK))
    {
        tcp_client_process_data(sk, data, ip_hdr, tcp_hdr);
    }
    else if (flags == (TH_SYN | TH_ACK))
    {
        tcp_client_process_syn_ack(sk, data, ip_hdr, tcp_hdr);
    }
    else if (flags & TH_RST)
    {
        tcp_process_rst(sk, data);
    }
    else
    {
        // SOCKET_LOG_ENABLE(sk);
        // MBUF_LOG(m, "drop-bad-flags");
        // SOCKET_LOG(sk, "drop-bad-flags");
        net_stats_tcp_drop();
        mbuf_free2(data);
    }
}

int TCP::process(Socket *sk, rte_mbuf *data)
{
    ether_header *eth_hdr = rte_pktmbuf_mtod(data, struct ether_header *);
    iphdr *ip_hdr = rte_pktmbuf_mtod_offset(data, struct iphdr *, sizeof(ether_header));
    tcphdr *tcp_hdr = rte_pktmbuf_mtod_offset(data, struct tcphdr *, sizeof(ether_header)+sizeof(iphdr));
    int olen = rte_pktmbuf_data_len(data);

    TCP *tcp = this;

    net_stats_tcp_rx();

    if (socket == nullptr)
    {
        if (TCP::g_vars[sk->pattern].global_tcp_rst)
        {
            tcp_reply_rst(sk, eth_hdr, ip_hdr, tcp_hdr, data, olen);
        }
        else
        {
            net_stats_tcp_drop();
            mbuf_free2(data);
        }
        return 0;
    }

    if (tcp->g_vars[sk->pattern].server)
    {
        tcp_server_process(sk, eth_hdr, ip_hdr, tcp_hdr, data, olen);
    }
    else
    {
        tcp_client_process(sk, eth_hdr, ip_hdr, tcp_hdr, data, olen);
    }
    return 0;
}

Socket *tcp_new_socket(const Socket *template_socket)
{
    Socket *socket = new Socket(*template_socket);
#ifdef DEBUG_
    printf("Thread: %d, tcp_new_socket: %p\n", g_config_percore->lcore_id, socket);
#endif
    return socket;
}

void tcp_release_socket(Socket *socket)
{
    delete socket;
}

void tcp_validate_socket(Socket *socket)
{
    TCP *tcp = &socket->tcp;
    tcp->state = TCP_CLOSE;
    tcp->timer.timer_tsc = time_in_config();
    // tcp->timer.data = socket;
    unique_timer_init(&tcp->timer);
    tcp->snd_nxt = rand_();
    tcp->snd_una = tcp->snd_nxt;
    tcp->keepalive_request_num = 0;
    tcp->flags = 0;
    tcp->keepalive = TCP::g_vars[socket->pattern].global_keepalive;
    socket->protocol = IPPROTO_TCP;
#ifdef DEBUG_
    printf("Thread: %d, tcp_validate_socket: %p\n", g_config_percore->lcore_id, socket);
#endif
}

void tcp_validate_csum(Socket *socket)
{
    TCP *tcp = &socket->tcp;
    tcp->csum_tcp = csum_pseudo_ipv4(IPPROTO_TCP, ntohl(socket->src_addr), ntohl(socket->dst_addr), TCP::g_vars[socket->pattern].template_tcp_pkt->data.l4_len + TCP::g_vars[socket->pattern].template_tcp_pkt->data.data_len);
    tcp->csum_tcp_data = csum_pseudo_ipv4(IPPROTO_TCP, ntohl(socket->src_addr), ntohl(socket->dst_addr), TCP::g_vars[socket->pattern].template_tcp_data->data.l4_len + TCP::g_vars[socket->pattern].template_tcp_data->data.data_len);
    tcp->csum_tcp_opt = csum_pseudo_ipv4(IPPROTO_TCP, ntohl(socket->src_addr), ntohl(socket->dst_addr), TCP::g_vars[socket->pattern].template_tcp_opt->data.l4_len + TCP::g_vars[socket->pattern].template_tcp_opt->data.data_len);
}

void tcp_validate_csum_opt(Socket *socket)
{
    TCP *tcp = &socket->tcp;
    tcp->csum_tcp_opt = csum_pseudo_ipv4(IPPROTO_TCP, ntohl(socket->src_addr), ntohl(socket->dst_addr), TCP::g_vars[socket->pattern].template_tcp_opt->data.l4_len + TCP::g_vars[socket->pattern].template_tcp_opt->data.data_len);
}

void tcp_launch(Socket *socket)
{
    TCP *tcp = &socket->tcp;
    tcp_validate_socket(socket);
    tcp->state = TCP_SYN_SENT;
    tcp_reply(socket, TH_SYN);
    net_stats_socket_open();
#ifdef DEBUG_
    printf("Thread: %d, tcp_launch_socket: %p\n", g_config_percore->lcore_id, socket);
#endif
}