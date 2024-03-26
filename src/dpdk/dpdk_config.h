#ifndef DPDK_CONFIG_H_
#define DPDK_CONFIG_H_

#include "common/define.h"
#include "dpdk/port.h"
#include "dpdk/dpdk_config_user.h"
#include "common/utils.h"
#include "arpa/inet.h"
#include "common/packets.h"
#include "timer/timer.h"
#include "panel/panel.h"

struct dpdk_config;
struct dpdk_config_percore;

extern __thread dpdk_config_percore *g_config_percore;
extern dpdk_config *g_config;

struct dpdk_config
{
    int num_lcores;
    int lcores[MAX_LCORES];
    uint8_t rss;
    bool mq_rx_rss;
    uint8_t rss_auto;
    bool jumbo;
    bool always_accurate_time;
    uint16_t jumbo_mtu;
    uint16_t vlan_id;
    int num_rx_queues;
    int num_tx_queues;
    int num_ports;
    struct netif_port ports[NETIF_PORT_MAX];
    char socket_mem[RTE_ARG_LEN];
    int tx_burst_size;
    int rx_burst_size;
    dpdk_config(dpdk_config_user *user_config);
};

struct dpdk_config_percore
{
    int epoch_work;
    uint64_t epoch_last_tsc;

    uint8_t lcore_id;
    uint8_t port_id;
    uint8_t queue_id;
    struct netif_port *port;
    rx_queue *rq;
    tx_queue *tq;
    tick_time time;
    cpuload cpusage;
    dpdk_config_percore(dpdk_config *config);

    static inline void cfg_send_packet(struct rte_mbuf *mbuf)
    {
        net_stats_tx(mbuf);
        send_packet_by_queue(g_config_percore->tq, mbuf, g_config_percore->port_id, g_config_percore->queue_id);
    }

    static inline rte_mbuf *cfg_recv_packet()
    {
        rte_mbuf* m = recv_packet_by_queue(g_config_percore->rq, g_config_percore->port_id, g_config_percore->queue_id);
        if (m)
        {
            net_stats_rx(m);
            g_config_percore->epoch_work++;
        }
        return m;
    }

    static inline void cfg_send_flush()
    {
        tx_queue_flush(g_config_percore->tq, g_config_percore->port_id, g_config_percore->queue_id);
    }

    static bool check_epoch_timer(uint64_t gap_tsc);
};

uint64_t& time_in_config();

#endif /* DPDK_CONFIG_H_ */