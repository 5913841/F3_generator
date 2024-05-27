#ifndef TCP_H
#define TCP_H

// the file implements the TCP protocol based on IPv4 and ethernet, you can modify or write your own TCP protocol based on other l3 protocols.

#include <netinet/tcp.h>
#include "dpdk/mbuf_template.h"
#include <functional>
#include "timer/rand.h"
#include "timer/unique_timer.h"

struct SocketPointerTable;
struct FiveTuples;

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

struct global_tcp_vars
{
    bool flood;
    bool server;
    uint8_t send_window;
    bool global_keepalive;
    bool global_stop;
    /* tsc */
    uint64_t keepalive_request_interval;
    int setted_keepalive_request_num;
    // static std::function<void(Socket *sk)> release_socket_callback;
    // static std::function<void(Socket *sk)> create_socket_callback;
    // static std::function<bool(FiveTuples ft, Socket *sk)> checkvalid_socket_callback;
    // static __thread void(*release_socket_callback)(Socket *sk);
    // static __thread void(*create_socket_callback)(Socket *sk);
    // static __thread bool(*checkvalid_socket_callback)(FiveTuples ft, Socket *sk);
    bool global_tcp_rst;
    uint8_t tos;
    bool use_http;
    uint16_t global_mss;
    bool constructing_opt_tmeplate;
    SocketPointerTable* socket_table;
    bool preset;
};

struct global_tcp_templates{
    mbuf_cache *template_tcp_data;
    mbuf_cache *template_tcp_opt;
    mbuf_cache *template_tcp_pkt;
};

class TCP
{
public:
    UniqueTimer timer;
    uint32_t rcv_nxt;
    uint32_t snd_nxt;
    uint32_t snd_una;
    uint32_t snd_wnd;
    uint16_t log : 1;
    uint16_t keepalive_request_num : 15;
    uint16_t csum_tcp;
    uint16_t csum_tcp_opt;
    uint16_t csum_tcp_data;
    // uint16_t csum_ip;
    // uint16_t csum_ip_opt;
    // uint16_t csum_ip_data;
    TCP_STATE state : 4;
    uint8_t retrans : 3;
    uint8_t keepalive : 1;
    uint8_t flags; /* tcp flags*/
    static thread_local SocketPointerTable* socket_table;

    static tcphdr *decode_hdr_pre(rte_mbuf *data)
    {
        return rte_pktmbuf_mtod_offset(data, struct tcphdr *, -sizeof(struct tcphdr));
    }

    size_t get_hdr_len(Socket *socket, rte_mbuf *data)
    {
        return sizeof(struct tcphdr);
    }

    int construct(Socket *socket, rte_mbuf *data);

    int process(Socket *socket, rte_mbuf *data);

    static void timer_init(int pattern);

    static void tcp_init(int pattern)
    {
        srand_(rte_rdtsc());
        timer_init(pattern);
    }
};

void tcp_reply(struct Socket *sk, uint8_t tcp_flags);
void tcp_start_keepalive_timer(struct Socket *sk, uint64_t now_tsc);

Socket *tcp_new_socket(const Socket *template_socket);

void tcp_release_socket(Socket *socket);

void tcp_launch(Socket *socket);

void tcp_validate_socket(Socket *socket);

void tcp_validate_csum(Socket* scoket);

void tcp_validate_csum_opt(Socket* scoket);

void tcp_validate_csum_pkt(Socket *socket);

void tcp_validate_csum_data(Socket *socket);
#endif