#include <iostream>
#include <stdint.h>
#include <inttypes.h>
#include "common/primitives.h"

using namespace primitives;

// #define RTE_PKTMBUF_HEADROOM 256
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

ip4addr_t target_ip("10.233.1.1");

dpdk_config_user usrconfig = {
    .lcores = {0},
    .ports = {"0000:01:00.1"},
    .gateway_for_ports = {"90:e2:ba:87:62:98"},
    .queue_num_per_port = {1},
    .always_accurate_time = false,
    .tx_burst_size = 8,
    .rx_burst_size = 8,
};

protocol_config p_config = {
    .protocol = "TCP",
    .mode = "client",
    .preset = true,
    .use_http = true,
    .use_keepalive = false,
    .cps = "1.2M",
};

int main(int argc, char **argv)
{
    set_configs_and_init(usrconfig, argv);

    set_pattern_num(1);

    add_pattern(p_config);

    ip4addr_t base_src = ip4addr_t("10.233.1.0");
    ip4addr_t base_dst = ip4addr_t("10.234.1.0");
    srand_(2024);
    for(int i = 0; i < 10000000; i++)
    {
        FiveTuples fivetuples;
        fivetuples.dst_port = rand_() % 20 + 1;
        fivetuples.src_port = rand_();
        fivetuples.dst_addr = rand_() % 256 + base_dst;
        fivetuples.src_addr = rand_() % 256 + base_src;
        add_fivetuples(fivetuples, 0);
    }

    run_setted();
}