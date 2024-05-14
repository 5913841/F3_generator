#ifndef DPDK_CONFIG_H_
#define DPDK_CONFIG_H_

#include "common/define.h"
#include "dpdk/port.h"
#include "dpdk/dpdk_config_user.h"
#include "common/utils.h"
#include "arpa/inet.h"
#include "common/packets.h"
#include "timer/clock.h"
#include "panel/panel.h"
#include "common/rate_control.h"
#include <rte_ethdev.h>

struct dpdk_config;
struct dpdk_config_percore;

extern __thread dpdk_config_percore *g_config_percore;
extern dpdk_config *g_config;

struct dpdk_config
{
    int num_lcores;
    int lcores[MAX_LCORES];
    bool jumbo;
    bool always_accurate_time;

    bool use_fdir;
    int addr_family;
    uint32_t src_range_mask;
    uint32_t dst_range_mask;
    uint32_t src_base_ip;
    uint32_t dst_base_ip;

    bool use_rss;
    uint8_t rss_type;
    bool mq_rx_rss;
    uint8_t rss_auto;

    uint16_t jumbo_mtu;
    uint16_t vlan_id;
    int num_rx_queues;
    int num_tx_queues;
    int num_ports;
    struct netif_port ports[NETIF_PORT_MAX];
    char socket_mem[RTE_ARG_LEN];
    int tx_burst_size;
    int rx_burst_size;
    const bool use_clear_nic_queue = false;
    dpdk_config(dpdk_config_user *user_config);
};

uint64_t& time_in_config();
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
    launch_control launch_ctls[MAX_PATTERNS];
private:
    uint64_t clear_nic_queue_next;
public:
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

    static void enter_epoch()
    {
        tick_time_update(&g_config_percore->time);
        CPULOAD_ADD_TSC(&g_config_percore->cpusage, time_in_config(), g_config_percore->epoch_work);

        if (unlikely(tsc_time_go(&g_config_percore->time.second, time_in_config())))
        {
            net_stats_timer_handler();
            if(g_config_percore->lcore_id == 0)
            {
                net_stats_print_speed(0, g_config_percore->time.second.count);
            }
#ifdef CLEAR_NIC
            if(unlikely(g_config->use_clear_nic_queue && (g_config_percore->clear_nic_queue_next <= time_in_config()) && (g_eth_stats.ierrors > 1000000))){
                rte_eth_dev_rx_queue_stop(g_config_percore->port_id, g_config_percore->queue_id);
                rte_eth_dev_tx_queue_start(g_config_percore->port_id, g_config_percore->queue_id);
                rte_eth_stats_reset(g_config_percore->port_id);
                g_config_percore->clear_nic_queue_next += 10 * g_tsc_per_second;
            }
#endif
        }
    }

    static void time_update()
    {
        tick_time_update(&g_config_percore->time);
    }

    static inline int check_epoch_timer(int pattern)
    {
        int launch_num = get_launch_num(g_config_percore->launch_ctls+pattern);
        g_config_percore->epoch_work += launch_num;

        return launch_num;
    }
};


#endif /* DPDK_CONFIG_H_ */