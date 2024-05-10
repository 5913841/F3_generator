#include <iostream>
#include <stdint.h>
#include <inttypes.h>
#include "protocols/conf_protocol.h"

// #define RTE_PKTMBUF_HEADROOM 256
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)


ip4addr_t target_ip("10.233.1.1");

dpdk_config_user usrconfig = {
    .lcores = {0},
    .ports = {"0000:01:00.0"},
    .gateway_for_ports = {"90:e2:ba:8a:c7:a1"},
    .queue_num_per_port = {1},
    .always_accurate_time = false,
    .tx_burst_size = 8,
    .rx_burst_size = 2048,
};

dpdk_config config = dpdk_config(&usrconfig);

protocol_config p_config = {
    .protocol = "TCP",
    .mode = "server",
    .use_http = true,
    .use_keepalive = false,
    .cps = "1M",
};

int start_test(__rte_unused void *arg1)
{
    dpdk_config_percore::enter_epoch();
    ip4addr_t base_src = ip4addr_t("10.233.1.2");
    uint64_t begin_ts = current_ts_msec();
    while (true)
    {
        rte_mbuf *m = nullptr;

        m = dpdk_config_percore::cfg_recv_packet();

        if (m != nullptr)
        {
            Socket* parse_socket = parse_packet(m);
            Socket *socket = TCP::socket_table->find_socket(parse_socket);
            if(socket == nullptr)
            {
                socket = tcp_new_socket(template_socket);
                memcpy(socket, parse_socket, sizeof(FiveTuples));
            }
            socket->tcp.process(socket, m);
        }

        dpdk_config_percore::cfg_send_flush();
        http_ack_delay_flush();

        // if (current_ts_msec() - begin_ts > 10 * 1000)
        // {
        //     break;
        // }
        TIMERS.trigger();
    }
    return 0;
}

int main(int argc, char **argv)
{
    dpdk_init(&config, argv[0]);
    int lcore_id = 0;
    TCP::pattern_num = 1;
    HTTP::pattern_num = 1;
    config_protocols(0, &p_config);
    start_test(NULL);
}