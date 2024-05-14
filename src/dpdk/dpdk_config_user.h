#ifndef _DPDK_CONFIG_USER_H_
#define _DPDK_CONFIG_USER_H_

#include "common/define.h"
#include "dpdk/port.h"
#include <string>
#include <vector>

struct dpdk_config_user
{
    std::vector<int> lcores;
    std::vector<std::string> ports;
    std::vector<std::string> gateway_for_ports; // ip or mac
    std::vector<int> queue_num_per_port;
    std::string socket_mem;
    bool always_accurate_time;
    std::string flow_distribution_strategy;
    std::string rss_type;
    std::string rss_auto_type;
    int addr_family;
    uint32_t src_range_mask;
    uint32_t dst_range_mask;
    uint32_t src_base_ip;
    uint32_t dst_base_ip;
    bool mq_rx_rss;
    bool mq_rx_none;
    int tx_burst_size;
    int rx_burst_size;
#ifdef CLEAR_NIC
    const bool use_clear_nic_queue = false;
#endif
};

#endif /* _DPDK_CONFIG_USER_H_ */
