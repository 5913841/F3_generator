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
    .use_preset_flowtable_size = false,
    .tx_burst_size = 64,
    .rx_burst_size = 8,
};

protocol_config p_config = {
    .protocol = "TCP",
    .mode = "client",
    .preset = false,
    .use_http = false,
    .use_keepalive = true,
    .keepalive_interval = "30s",
    .keepalive_request_maxnum = "1",
    .launch_batch = "10",
    .cps = "0.3M",
    .cc = "7M",
    .template_ip_src = "10.233.1.2",
    .template_ip_dst = "10.233.1.3"
};

void random(Socket* socket, void* data)
{
    socket->dst_port = rand_();
    socket->src_port = rand_();
    socket->src_addr = socket->src_addr + rand_() % 11;
}
int main(int argc, char **argv)
{

    set_configs_and_init(usrconfig, argv);

    set_pattern_num(1);

    add_pattern(p_config);

    set_random_method(random, 0, NULL);

    run_generate();
}