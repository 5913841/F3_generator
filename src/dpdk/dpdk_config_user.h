#ifndef _DPDK_CONFIG_USER_H_
#define _DPDK_CONFIG_USER_H_

#include "common/define.h"
#include "dpdk/port.h"
#include <string>
#include <vector>

struct dpdk_config_user {
    std::vector<int> lcores;
    std::vector<std::string> ports;
    std::vector<std::string> gateway_for_ports; //ip or mac
    std::vector<int> queue_num_per_port;
    std::string socket_mem;

};

#endif /* _DPDK_CONFIG_USER_H_ */
