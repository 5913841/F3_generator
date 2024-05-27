#include <iostream>
#include <stdint.h>
#include <sstream>
#include <vector>
#include <string>
#include "common/primitives.h"

using namespace primitives;

// #define RTE_PKTMBUF_HEADROOM 256
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

uint32_t mod_sa = 1100;
uint32_t mod_da = 1003;
uint16_t mod_sp = 1020;
uint16_t mod_dp = 1100;

dpdk_config_user usrconfig = {
    .lcores = {0},
    .ports = {"0000:01:00.0"},
    .gateway_for_ports = {"90:e2:ba:8a:c7:a1"},
    .queue_num_per_port = {1},
    .always_accurate_time = false,
    .tx_burst_size = 128,
    .rx_burst_size = 64,
};

protocol_config p_config = {
    .protocol = "UDP",
    .mode = "client",
    .preset = true,
    .launch_batch = "1000",
    .payload_size = "1",
    .cps = "20M",
    .cc = "16k",
    .template_ip_src = "10.233.1.2",
    .template_ip_dst = "10.233.1.3"
};

void random(Socket* socket)
{
    // socket->dst_port = rand_() % mod_dp;
    // socket->src_port = rand_() % mod_sp;
    // socket->src_addr = rand_() % mod_sa;
    // socket->dst_addr = rand_() % mod_da;
}
// argv[1] = lcores
// argv[2] = mod_sa
// argv[3] = mod_da
// argv[4] = mod_sp
// argv[5] = mod_dp
int main(int argc, char **argv)
{
    set_configs_and_init(usrconfig, argv);

    set_pattern_num(1);

    add_pattern(p_config);

    Socket* socket = new Socket(*template_socket);
    for(int i = 0; i < 1000000; i++)
    {
        random(socket);
        add_fivetuples(*(FiveTuples*)socket, 0);
    }

    run_generate();
}