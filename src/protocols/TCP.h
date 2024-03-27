#ifndef TCP_H
#define TCP_H

// the file implements the TCP protocol based on IPv4 and ethernet, you can modify or write your own TCP protocol based on other l3 protocols.

#include "protocols/base_protocol.h"
#include <netinet/tcp.h>
#include "dpdk/mbuf_template.h"
#include <functional>

bool tcp_seq_lt(uint32_t a, uint32_t b);
bool tcp_seq_le(uint32_t a, uint32_t b);
bool tcp_seq_gt(uint32_t a, uint32_t b);
bool tcp_seq_ge(uint32_t a, uint32_t b);

typedef uint8_t TCP_STATE;

struct tcp_opt_mss
{
    uint8_t kind;
    uint8_t len;
    uint16_t mss;
} __attribute__((__packed__));

class TCP;

extern TCP *parser_tcp;

class TCP : public L4_Protocol
{
public:
    uint32_t rcv_nxt;
    uint32_t snd_nxt;
    uint32_t snd_una;
    uint32_t snd_wnd;
    uint32_t snd_max;

    TCP_STATE state : 4;
    uint8_t retrans : 3;
    uint8_t keepalive : 1;
    uint8_t flags; /* tcp flags*/
    uint16_t log : 1;
    uint16_t keepalive_request_num : 15;
    uint64_t timer_tsc;
    // uint16_t csum_tcp;
    // uint16_t csum_tcp_opt;
    // uint16_t csum_tcp_data;
    // uint16_t csum_ip;
    // uint16_t csum_ip_opt;
    // uint16_t csum_ip_data;

    static bool flood;
    static bool server;
    static uint8_t send_window;
    static mbuf_cache *template_tcp_data;
    static mbuf_cache *template_tcp_opt;
    static mbuf_cache *template_tcp_pkt;
    static int global_duration_time;
    static bool global_keepalive;
    static bool global_stop;
    /* tsc */
    static uint64_t keepalive_request_interval;
    static int setted_keepalive_request_num;
    static std::function<void(Socket *sk)> release_socket_callback;
    static std::function<void(Socket *sk)> create_socket_callback;
    static bool global_tcp_rst;
    static uint8_t tos;
    static bool use_http;
    static uint16_t global_mss;
    static bool constructing_opt_tmeplate;

    TCP() : state(TCP_CLOSE), L4_Protocol(ProtocolCode::PTC_TCP){};
    static tcphdr *decode_hdr_pre(rte_mbuf *data)
    {
        return rte_pktmbuf_mtod_offset(data, struct tcphdr *, -sizeof(struct tcphdr));
    }
    size_t get_hdr_len(Socket *socket, rte_mbuf *data)
    {
        return sizeof(struct tcphdr);
    }
    int construct(Socket *socket, rte_mbuf *data)
    {
        int opt_len = 0;
        if (constructing_opt_tmeplate && global_mss != 0)
        {
            uint8_t wscale[4] = {1, 3, 3, DEFAULT_WSCALE};
            rte_pktmbuf_prepend(data, 4);
            rte_memcpy(rte_pktmbuf_mtod(data, uint8_t *), wscale, 4);
            opt_len += 4;
            tcp_opt_mss *opt_mss = rte_pktmbuf_mtod_offset(data, struct tcp_opt_mss *, -sizeof(struct tcp_opt_mss));
            opt_mss->kind = 2;
            opt_mss->len = 4;
            opt_mss->mss = htons(global_mss);
            opt_len += sizeof(struct tcp_opt_mss);
            rte_pktmbuf_prepend(data, sizeof(struct tcp_opt_mss));
        }
        tcphdr *tcp = decode_hdr_pre(data);
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
        tcp->window = snd_wnd;
        tcp->check = 0;
        tcp->urg_ptr = 0;
        data->l4_len = sizeof(struct tcphdr) + opt_len;
        rte_pktmbuf_prepend(data, sizeof(struct tcphdr));
        return sizeof(struct tcphdr) + opt_len;
    }
    int process(Socket *socket, rte_mbuf *data);

    static int parse_packet_ft(rte_mbuf *data, FiveTuples *ft, int offset)
    {
        ft->protocol_codes[SOCKET_L4] = ProtocolCode::PTC_TCP;
        tcphdr *tcp = rte_pktmbuf_mtod_offset(data, struct tcphdr *, offset);
        ft->src_port = ntohs(tcp->dest);
        ft->dst_port = ntohs(tcp->source);
        return sizeof(struct tcphdr);
    }

    static int parse_packet_sk(rte_mbuf *data, Socket *sk, int offset)
    {
        sk->l4_protocol = parser_tcp;
        tcphdr *tcp = rte_pktmbuf_mtod_offset(data, struct tcphdr *, offset);
        sk->src_port = ntohs(tcp->dest);
        sk->dst_port = ntohs(tcp->source);
        return sizeof(struct tcphdr);
    }

    static void parser_init()
    {
        L4_Protocol::parser.add_parser(parse_packet_ft);
        L4_Protocol::parser.add_parser(parse_packet_sk);
    }

    static void timer_init();
};

inline void tcp_launch();
struct rte_mbuf *tcp_reply(TCP *tcp, struct Socket *sk, uint8_t tcp_flags);
static inline void tcp_start_keepalive_timer(TCP *tcp, struct Socket *sk, uint64_t now_tsc);

Socket *tcp_new_socket(const Socket *template_socket);

void tcp_release_socket(Socket *socket);

void tcp_launch(Socket *socket);

#endif