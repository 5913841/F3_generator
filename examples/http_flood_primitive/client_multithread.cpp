#include <iostream>
#include <stdint.h>
#include <inttypes.h>
#include "common/primitives.h"

using namespace primitives;

// #define RTE_PKTMBUF_HEADROOM 256
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)


dpdk_config_user usrconfig = {
    .lcores = {0, 1},
    .ports = {"0000:01:00.1"},
    .gateway_for_ports = {"90:e2:ba:87:62:98"},
    .queue_num_per_port = {2},
    .always_accurate_time = false,
    .tx_burst_size = 8,
    .rx_burst_size = 2048,
};

protocol_config p_config1 = {
    .protocol = "TCP",
    .mode = "client",
    .use_http = false,
    .use_keepalive = false,
    .cps = "0.23M",
    .template_ip_src = "10.223.1.2",
    .template_ip_dst = "10.223.1.3"
};

protocol_config p_config2 = {
    .protocol = "TCP",
    .mode = "client",
    .use_http = true,
    .use_keepalive = true,
    .keepalive_interval = "1s",
    .keepalive_request_maxnum = "2",
    .payload_size = "200",
    .cps = "0.23M",
    .template_ip_src = "10.233.1.2",
    .template_ip_dst = "10.233.1.3"
};


void random(Socket* socket)
{
    socket->dst_port = rand_() % 20 + 1;
    socket->src_port = rand_();
    socket->src_addr = socket->src_addr + rand_() % 11;
}

int main(int argc, char **argv)
{
    set_configs_and_init(usrconfig, argv);

    set_pattern_num(2);

    add_pattern(p_config1);

    add_pattern(p_config2);

    set_random_method(random, 0);

    set_random_method(random, 1);

    run_setted();
}