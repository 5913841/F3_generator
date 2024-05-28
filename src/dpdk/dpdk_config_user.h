#ifndef _DPDK_CONFIG_USER_H_
#define _DPDK_CONFIG_USER_H_

#include "common/define.h"
#include "dpdk/port.h"
#include <string>
#include <vector>

struct dpdk_config_user
{
    std::vector<int> lcores = {0};
    std::vector<std::string> ports;
    std::vector<std::string> gateway_for_ports; // ip or mac
    std::vector<int> queue_num_per_port = {1};
    std::string socket_mem = "";
    bool always_accurate_time = false;
    bool use_preset_flowtable_size = true;
    std::string flowtable_init_size = "0";
    std::string flow_distribution_strategy = "rss";
    std::string rss_type = "l3";
    std::string rss_auto_type = "";
    int addr_family;
    uint32_t src_range_mask;
    uint32_t dst_range_mask;
    uint32_t src_base_ip;
    uint32_t dst_base_ip;
    bool mq_rx_rss = true;
    bool mq_rx_none = false;
    int tx_burst_size = 64;
    int rx_burst_size = 64;
#ifdef CLEAR_NIC
    const bool use_clear_nic_queue = false;
#endif
};

#endif /* _DPDK_CONFIG_USER_H_ */
