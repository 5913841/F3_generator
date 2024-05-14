#include <iostream>
#include <stdint.h>
#include <sstream>
#include <vector>
#include <string>
#include "common/primitives.h"

using namespace primitives;

// #define RTE_PKTMBUF_HEADROOM 256
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)


dpdk_config_user usrconfig = {
    .lcores = {0},
    .ports = {"0000:01:00.1"},
    .gateway_for_ports = {"90:e2:ba:87:62:98"},
    .queue_num_per_port = {1},
    .always_accurate_time = false,
    .tx_burst_size = 8,
    .rx_burst_size = 2048,
};
// 1core - 1.37M, 

protocol_config p_config = {
    .protocol = "TCP",
    .mode = "client",
    .use_http = false,
    .use_keepalive = false,
    .cps = "0",
    .template_ip_src = "10.233.1.2",
    .template_ip_dst = "10.233.1.3"
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

void random(Socket* socket)
{
    socket->dst_port = rand_() % 20 + 1;
    socket->src_port = rand_();
    socket->src_addr = socket->src_addr + rand_() % 11;
}
// argv[1] = lcores
// argv[2] = cps
int main(int argc, char **argv)
{
    usrconfig.lcores = parseStringToIntVector(argv[1]);
    usrconfig.queue_num_per_port = {((int)usrconfig.lcores.size())};
    if (usrconfig.lcores.size() > 1)
    {
        usrconfig.flow_distribution_strategy = "rss";
        usrconfig.rss_type = "l3";
        usrconfig.mq_rx_rss = true;
    }

    p_config.cps = argv[2];

    set_configs_and_init(usrconfig, argv);

    set_pattern_num(1);

    add_pattern(p_config);

    set_random_method(random, 0);

    run_setted();
}