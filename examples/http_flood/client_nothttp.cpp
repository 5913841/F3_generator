#include <iostream>
#include <stdint.h>
#include <inttypes.h>
#include "protocols/conf_protocol.h"

// #define RTE_PKTMBUF_HEADROOM 256
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

ETHER *ether = new ETHER();
IPV4 *ip = new IPV4();
TCP *template_tcp = new TCP();
HTTP *template_http = new HTTP();
Socket *template_socket = new Socket();
mbuf_cache *template_tcp_data = new mbuf_cache();
mbuf_cache *template_tcp_opt = new mbuf_cache();
mbuf_cache *template_tcp_pkt = new mbuf_cache();
const char *data;
// SocketPointerTable *socket_table = new SocketPointerTable();

ip4addr_t target_ip("10.233.1.1");

dpdk_config_user usrconfig = {
    .lcores = {0},
    .ports = {"0000:01:00.1"},
    .gateway_for_ports = {"90:e2:ba:87:62:98"},
    .queue_num_per_port = {1},
    .always_accurate_time = false,
    .tx_burst_size = 8,
    .rx_burst_size = 2048,
};

dpdk_config config = dpdk_config(&usrconfig);

protocol_config p_config = {
    .protocol = "TCP",
    .mode = "client",
    .use_http = false,
    .use_keepalive = false,
    .cps = "1M",
};

int start_test(__rte_unused void *arg1)
{
    ip4addr_t base_src = ip4addr_t("10.233.1.2");
    uint64_t begin_ts = current_ts_msec();
    while (true)
    {
        do
        {
            rte_mbuf *m = dpdk_config_percore::cfg_recv_packet();
            tick_time_update(&g_config_percore->time);
            if (!m)
            {
                break;
            }
            Socket* parse_socket = parse_packet(m);
            Socket *socket = TCP::socket_table->find_socket(parse_socket);
            if (socket != nullptr)
                socket->tcp.process(socket, m);
        } while (true);
        dpdk_config_percore::cfg_send_flush();

        // if (current_ts_msec() - begin_ts > 10 * 1000)
        // {
        //     break;
        // }

        if (dpdk_config_percore::check_epoch_timer(0.0000007 * TSC_PER_SEC))
        {
            for (int i = 0; i < 3; i++)
            {
                Socket *socket = tcp_new_socket(template_socket);

                socket->dst_port = rand() % 20 + 1;
                socket->src_port = rand();
                socket->src_addr = rand() % 11 + base_src;
                if (TCP::socket_table->insert_socket(socket) == -1)
                {
                    tcp_release_socket(socket);
                    continue;
                }
                tcp_validate_csum(socket);
                tcp_launch(socket);
            }
            TIMERS.trigger();
        }
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