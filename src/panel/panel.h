#ifndef PANEL_H
#define PANEL_H
#include <stdint.h>
#include <stdio.h>
#include <rte_ethdev.h>
#include "common/define.h"

/*
 * Each thread calculates CPU usage every second.
 * cpu-usage = working-time / 1-seconds
 * */
struct cpuload
{
    uint64_t init_tsc;  /* updated every second */
    uint64_t start_tsc; /* update after accumulation */
    uint64_t work_tsc;  /* accumulate in one second */
};

#define CPULOAD_ADD_TSC(load, now_tsc, work)                   \
    do                                                         \
    {                                                          \
        if (work)                                              \
        {                                                      \
            (load)->work_tsc += (now_tsc) - (load)->start_tsc; \
        }                                                      \
        (load)->start_tsc = (now_tsc);                         \
        work = 0;                                              \
    } while (0)

void cpuload_init(struct cpuload *load);
uint32_t cpuload_cal_cpusage(struct cpuload *load, uint64_t now_tsc);

struct net_stats
{
    /* Increasing */

    /* interface */
    uint64_t pkt_rx;
    uint64_t pkt_tx;
    uint64_t byte_rx;
    uint64_t byte_tx;

    uint64_t tx_drop;
    uint64_t pkt_lost;
    uint64_t rx_bad;

    uint64_t tos_rx;

    /* socket */
    uint64_t socket_dup[MAX_PATTERNS];
    uint64_t socket_open[MAX_PATTERNS];
    uint64_t socket_close[MAX_PATTERNS];
    uint64_t socket_error[MAX_PATTERNS];

    /* tcp */
    uint64_t tcp_rx;
    uint64_t tcp_tx;

    uint64_t syn_rx;
    uint64_t syn_tx;

    uint64_t fin_rx;
    uint64_t fin_tx;

    uint64_t rst_rx;
    uint64_t rst_tx;

    uint64_t syn_rt;
    uint64_t fin_rt;
    uint64_t ack_rt;
    uint64_t push_rt;
    uint64_t ack_dup;

    uint64_t http_2xx;
    uint64_t tcp_req;
    uint64_t http_get;
    uint64_t tcp_rsp;
    uint64_t http_error;

    uint64_t tcp_drop;

    /* udp */
    uint64_t udp_rx;
    uint64_t udp_tx;
    uint64_t udp_rt;
    uint64_t udp_drop;

    /* arp */
    uint64_t arp_rx;
    uint64_t arp_tx;

    /* icmp and icmp6 */
    uint64_t icmp_rx;
    uint64_t icmp_tx;

    /* kni */
    uint64_t kni_rx;
    uint64_t kni_tx;

    uint64_t other_rx;

    /* mutable  */
    uint64_t mutable_start[0];

    uint64_t rtt_tsc;
    uint64_t rtt_num;

    uint64_t cpusage;
    uint64_t socket_current[MAX_PATTERNS];
};

void net_stats_init();
void net_stats_timer_handler();
void net_stats_print_total(FILE *fp);
void net_stats_print_speed(FILE *fp, int seconds);
extern __thread struct net_stats g_net_stats;
extern rte_eth_stats g_eth_stats;

#define net_stats_socket_dup(pattern)        \
    do                                \
    {                                 \
        g_net_stats.socket_dup[pattern]++;     \
        g_net_stats.socket_open[pattern]++;    \
        g_net_stats.socket_current[pattern]++; \
    } while (0)
#define net_stats_socket_error(pattern)    \
    do                              \
    {                               \
        g_net_stats.socket_error[pattern]++; \
    } while (0)
#define net_stats_socket_open(pattern)       \
    do                                \
    {                                 \
        g_net_stats.socket_open[pattern]++;    \
        g_net_stats.socket_current[pattern]++; \
    } while (0)
#define net_stats_socket_close(pattern)      \
    do                                \
    {                                 \
        g_net_stats.socket_close[pattern]++;   \
        g_net_stats.socket_current[pattern]--; \
    } while (0)
#define net_stats_tx_drop(n)        \
    do                              \
    {                               \
        g_net_stats.tx_drop += (n); \
    } while (0)
#define net_stats_rx_bad()    \
    do                        \
    {                         \
        g_net_stats.rx_bad++; \
    } while (0)

#define net_stats_udp_rt()    \
    do                        \
    {                         \
        g_net_stats.udp_rt++; \
    } while (0)
#define net_stats_syn_rt()    \
    do                        \
    {                         \
        g_net_stats.syn_rt++; \
    } while (0)
#define net_stats_fin_rt()    \
    do                        \
    {                         \
        g_net_stats.fin_rt++; \
    } while (0)
#define net_stats_ack_rt()    \
    do                        \
    {                         \
        g_net_stats.ack_rt++; \
    } while (0)
#define net_stats_push_rt()    \
    do                         \
    {                          \
        g_net_stats.push_rt++; \
    } while (0)
#define net_stats_ack_dup()    \
    do                         \
    {                          \
        g_net_stats.ack_dup++; \
    } while (0)

#define net_stats_pkt_lost()    \
    do                          \
    {                           \
        g_net_stats.pkt_lost++; \
    } while (0)

#define net_stats_tcp_req()    \
    do                         \
    {                          \
        g_net_stats.tcp_req++; \
    } while (0)
#define net_stats_tcp_rsp()    \
    do                         \
    {                          \
        g_net_stats.tcp_rsp++; \
    } while (0)
#define net_stats_http_2xx()    \
    do                          \
    {                           \
        g_net_stats.http_2xx++; \
    } while (0)
#define net_stats_http_error()    \
    do                            \
    {                             \
        g_net_stats.http_error++; \
    } while (0)
#define net_stats_http_get()    \
    do                          \
    {                           \
        g_net_stats.http_get++; \
    } while (0)
#define net_stats_fin_rx()    \
    do                        \
    {                         \
        g_net_stats.fin_rx++; \
    } while (0)
#define net_stats_fin_tx()    \
    do                        \
    {                         \
        g_net_stats.fin_tx++; \
    } while (0)
#define net_stats_syn_rx()    \
    do                        \
    {                         \
        g_net_stats.syn_rx++; \
    } while (0)
#define net_stats_syn_tx()    \
    do                        \
    {                         \
        g_net_stats.syn_tx++; \
    } while (0)
#define net_stats_rst_rx()    \
    do                        \
    {                         \
        g_net_stats.rst_rx++; \
    } while (0)
#define net_stats_rst_tx()    \
    do                        \
    {                         \
        g_net_stats.rst_tx++; \
    } while (0)
#define net_stats_tcp_drop()    \
    do                          \
    {                           \
        g_net_stats.tcp_drop++; \
    } while (0)
#define net_stats_udp_drop()    \
    do                          \
    {                           \
        g_net_stats.udp_drop++; \
    } while (0)
#define net_stats_arp_rx()    \
    do                        \
    {                         \
        g_net_stats.arp_rx++; \
    } while (0)
#define net_stats_arp_tx()    \
    do                        \
    {                         \
        g_net_stats.arp_tx++; \
    } while (0)
#define net_stats_icmp_rx()    \
    do                         \
    {                          \
        g_net_stats.icmp_rx++; \
    } while (0)
#define net_stats_icmp_tx()    \
    do                         \
    {                          \
        g_net_stats.icmp_tx++; \
    } while (0)
#define net_stats_kni_rx()    \
    do                        \
    {                         \
        g_net_stats.kni_rx++; \
    } while (0)
#define net_stats_kni_tx()    \
    do                        \
    {                         \
        g_net_stats.kni_tx++; \
    } while (0)
#define net_stats_tcp_rx()    \
    do                        \
    {                         \
        g_net_stats.tcp_rx++; \
    } while (0)
#define net_stats_tcp_tx()    \
    do                        \
    {                         \
        g_net_stats.tcp_tx++; \
    } while (0)
#define net_stats_udp_rx()    \
    do                        \
    {                         \
        g_net_stats.udp_rx++; \
    } while (0)
#define net_stats_udp_tx()    \
    do                        \
    {                         \
        g_net_stats.udp_tx++; \
    } while (0)
#define net_stats_other_rx()    \
    do                          \
    {                           \
        g_net_stats.other_rx++; \
    } while (0)
#define net_stats_rx(m)                                 \
    do                                                  \
    {                                                   \
        g_net_stats.pkt_rx++;                           \
        g_net_stats.byte_rx += rte_pktmbuf_data_len(m); \
    } while (0)

#define net_stats_tx(m)                                 \
    do                                                  \
    {                                                   \
        g_net_stats.pkt_tx++;                           \
        g_net_stats.byte_tx += rte_pktmbuf_data_len(m); \
    } while (0)
#define net_stats_rtt(tcp)                                       \
    do                                                           \
    {                                                            \
        g_net_stats.rtt_num++;                                   \
        g_net_stats.rtt_tsc += time_in_config() - tcp->timer.timer_tsc; \
    } while (0)

#define net_stats_tos_ipv4_rx(iph)              \
    do                                          \
    {                                           \
        if (TCP::tos && (TCP::tos == iph->tos)) \
        {                                       \
            g_net_stats.tos_rx++;               \
        }                                       \
    } while (0)

#define net_stats_tos_ipv6_rx(ip6h)                      \
    do                                                   \
    {                                                    \
        uint32_t mask = htonl(((uint32_t)0xffff) << 20); \
        if (ip6h->ip6_flow & mask)                       \
        {                                                \
            g_net_stats.tos_rx++;                        \
        }                                                \
    } while (0)

#endif