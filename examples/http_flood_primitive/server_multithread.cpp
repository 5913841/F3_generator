#include <iostream>
#include <stdint.h>
#include <inttypes.h>
#include "common/primitives.h"

// #define RTE_PKTMBUF_HEADROOM 256
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

using namespace primitives;

dpdk_config_user usrconfig = {
    .lcores = {0, 1},
    .ports = {"0000:01:00.0"},
    .gateway_for_ports = {"90:e2:ba:8a:c7:a1"},
    .queue_num_per_port = {2},
    .always_accurate_time = false,
    .tx_burst_size = 8,
    .rx_burst_size = 2048,
};

protocol_config p_config1 = {
    .protocol = "TCP",
    .mode = "server",
    .use_http = false,
    .use_keepalive = false,
    .cps = "0.3M",
};

protocol_config p_config2 = {
    .protocol = "TCP",
    .mode = "server",
    .use_http = true,
    .use_keepalive = true,
    .keepalive_interval = "1s",
    .keepalive_request_maxnum = "2",
    .payload_size = "100",
    .cps = "0.3M",
};

int main(int argc, char **argv)
{
    set_configs_and_init(usrconfig, argv);

    set_pattern_num(2);

    add_pattern(p_config1);

    add_pattern(p_config2);

    run_setted();
}