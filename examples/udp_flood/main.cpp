#include <iostream>
#include <stdint.h>
#include <inttypes.h>
#include "socket/socket.h"
#include "dpdk/dpdk.h"
#include "dpdk/dpdk_config.h"
#include "dpdk/dpdk_config_user.h"
#include "protocols/ETH.h"
#include "protocols/IP.h"
#include "protocols/UDP.h"
#include "protocols/HTTP.h"
#include "socket/socket_table/socket_table.h"
#include "socket/socket_tree/socket_tree.h"
#include "protocols/base_protocol.h"
#include <rte_lcore.h>

// #define RTE_PKTMBUF_HEADROOM 256
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

ETHER *ether = new ETHER();
IPV4 *ip = new IPV4();
UDP *udp = new UDP();
Socket *template_socket = new Socket();
mbuf_cache *template_udp_pkt = new mbuf_cache();
const char *data;

ipaddr_t target_ip("10.233.1.1");

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

void config_ether_variables()
{
    ETHER::parser_init();
    ether->port = config.ports + 0;
}

void config_ip_variables()
{
    IPV4::parser_init();
    return;
}

void config_udp_variables()
{
    UDP::release_socket_callback = [](Socket *sk)
    { UDP* udp = (UDP*)sk->l4_protocol; udp->state = TCP_CLOSE; };
    UDP::template_udp_pkt = template_udp_pkt;
    UDP::pipeline = 0;
    UDP::flood = true;
    UDP::global_duration_time =  60 * 1000;
    UDP::global_stop = false;
    UDP::payload_random = false;
    UDP::keepalive_request_interval = 0;
    udp->keepalive_request_num = 0;
    udp->keepalive = false;
    udp->timer_tsc = 0;
    udp_set_payload(64);
    data = udp_get_payload();
}

void config_socket()
{
    template_socket->src_addr = 0;
    template_socket->dst_addr = target_ip;
    template_socket->src_port = 0;
    template_socket->dst_port = 0;
    template_socket->set_protocol(SOCKET_L2, ether);
    template_socket->set_protocol(SOCKET_L3, ip);
    template_socket->set_protocol(SOCKET_L4, udp);
    template_socket->set_protocol(SOCKET_L5, nullptr);
}

void config_template_pkt()
{
    template_udp_pkt->mbuf_pool = mbuf_pool_create(&config, "template_udp_pkt", config.ports[0].id, 0);
    mbuf_template_pool_setby_socket(template_udp_pkt, template_socket, data, strlen(data));
}

int start_test(__rte_unused void *arg1)
{
    ipaddr_t base_src = ipaddr_t("10.233.1.2");
    uint64_t begin_ts = current_ts_msec();
    while (true)
    {
        tick_time_update(&g_config_percore->time);
        rte_mbuf *m = nullptr;
        Socket *socket = template_socket;

        socket->dst_port = rand_() % 2 ? 80 : 443;
        socket->src_port = rand_() % (65536 - 1024) + 1024;
        socket->src_addr = rand_();

        udp_launch(socket);

        dpdk_config_percore::cfg_send_flush();

        if (unlikely(tsc_time_go(&g_config_percore->time.second, time_in_config())))
        {
            net_stats_timer_handler();
            net_stats_print_speed(0, g_config_percore->time.second.count);
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    dpdk_init(&config, argv[0]);
    int lcore_id = 0;
    config_ether_variables();
    config_ip_variables();
    config_udp_variables();
    config_socket();
    config_template_pkt();
    start_test(NULL);
}