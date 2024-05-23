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
    .ports = {"0000:17:00.0", "0000:98:00.0"},
    .gateway_for_ports = {"08:c0:eb:58:92:d8", "08:c0:eb:42:5a:32"},
    .queue_num_per_port = {1, 1},
    .always_accurate_time = false,
    .tx_burst_size = 64,
    .rx_burst_size = 64,
};
// 1core - 1.63M(launch batch = 4), 2core - 2.5M(launch batch = 1 arg cps = 3M) 4core - 2.48M(launch batch = 1 arg cps = 3M)

protocol_config p_config = {
    .protocol = "TCP",
    .mode = "client",
    .preset = true,
    .use_http = false,
    .use_keepalive = false,
    .launch_batch = "1",
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
// argv[3] = launch_batch
// argv[4] = numqueues1,numqueues2
int main(int argc, char **argv)
{
    usrconfig.lcores = parseStringToIntVector(argv[1]);
    if (usrconfig.lcores.size() > 1)
    {
        usrconfig.flow_distribution_strategy = "rss";
        usrconfig.rss_type = "l3";
        usrconfig.mq_rx_rss = true;
    }

    p_config.cps = argv[2];
    p_config.launch_batch = argv[3];
    usrconfig.queue_num_per_port = parseStringToIntVector(argv[4]);

    set_configs_and_init(usrconfig, argv);

    set_pattern_num(1);

    add_pattern(p_config);

    Socket* socket = new Socket(*template_socket);
    // set_random_method(random, 0);
    for(int i = 0; i < 1000000; i++)
    {
        random(socket);
        add_fivetuples(*(FiveTuples*)socket, 0);
    }

    run_generate();
}