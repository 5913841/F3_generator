#ifndef _DPDK_CONFIG_USER_H_
#define _DPDK_CONFIG_USER_H_

#include "common/define.h"
#include "dpdk/port.h"
#include <string>
#include <vector>

struct dpdk_config_user
{
    std::vector<int> lcores = {0}; // vector of used lcores' id.
    std::vector<std::string> ports; // vector of used ports' pci name.
    std::vector<std::string> gateway_for_ports; // mac address of gateway for each port.
    std::vector<int> queue_num_per_port = {1}; // number of queues for each port. one cpu core for each queue
    // for example, if lcores = {0, 1, 2, 3}, queue_num_per_port = {3, 1}, then port 0 uses lcores 0, 1, 2, and port 1 uses lcores 3.
    std::string socket_mem = ""; // total hugepage memory for sockets.
    bool always_accurate_time = false; // whether to always use accurate time. if true, update clock when checked every time.
    bool use_preset_flowtable_size = true; // whether to use preset flowtable size.
    std::string flowtable_init_size = "0"; // if use preset flowtable size, initial flowtable size.
    std::string flow_distribution_strategy = "rss"; // Strategy of flow distribution, now we just use rss
    std::string rss_type = "l3"; // Choose the type of rss from l3, l3l4 or auto.
    std::string rss_auto_type = ""; // If use rss auto, choose l3 or l3l4 auto mode to use.
    int addr_family;
    uint32_t src_range_mask;
    uint32_t dst_range_mask;
    uint32_t src_base_ip;
    uint32_t dst_base_ip;
    bool mq_rx_rss = true; // Use ’RTE ETH MQ RX RSS’ to enable the rss on the NIC.
    bool mq_rx_none = false; // Do not use ’RTE ETH MQ RX RSS’ to enable the rss.
    int tx_burst_size = 64; // Number of packets sent in one time in NIC.
    int rx_burst_size = 64; // Number of packts reveived in one time in NIC.
#ifdef CLEAR_NIC
    const bool use_clear_nic_queue = false;
#endif
};

#endif /* _DPDK_CONFIG_USER_H_ */
