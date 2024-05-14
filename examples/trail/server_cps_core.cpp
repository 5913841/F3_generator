#include <iostream>
#include <stdint.h>
#include <sstream>
#include <vector>
#include <string>
#include "common/primitives.h"

// #define RTE_PKTMBUF_HEADROOM 256
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

using namespace primitives;

dpdk_config_user usrconfig = {
    .lcores = {0},
    .ports = {"0000:01:00.0"},
    .gateway_for_ports = {"90:e2:ba:8a:c7:a1"},
    .queue_num_per_port = {1}, 
    .always_accurate_time = false,
    .tx_burst_size = 8,
    .rx_burst_size = 2048,
    // .use_clear_nic_queue = true,
};

protocol_config p_config = {
    .protocol = "TCP",
    .mode = "server",
    .use_http = false,
    .use_keepalive = false,
    .cps = "1000M",
};

std::vector<int> parseStringToIntVector(const std::string& str) {
    std::vector<int> result;
    std::istringstream iss(str);
    std::string token;
    while (std::getline(iss, token, ',')) {
        try {
            int num = std::stoi(token);
            result.push_back(num);
        } catch (const std::invalid_argument& ia) {
            std::cerr << "Invalid argument: " << ia.what() << std::endl;
        } catch (const std::out_of_range& oor) {
            std::cerr << "Out of range error: " << oor.what() << std::endl;
        }
    }
    return result;
}

int main(int argc, char **argv)
{
    usrconfig.lcores = parseStringToIntVector(argv[1]);

    set_configs_and_init(usrconfig, argv);

    set_pattern_num(1);

    add_pattern(p_config);

    run_setted();
}