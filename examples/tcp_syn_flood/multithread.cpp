#include <iostream>
#include <stdint.h>
#include <inttypes.h>
#include "socket/socket.h"
#include "dpdk/dpdk.h"
#include "dpdk/dpdk_config.h"
#include "dpdk/dpdk_config_user.h"
#include "protocols/ETH.h"
#include "protocols/IP.h"
#include "protocols/TCP.h"
#include "protocols/HTTP.h"
#include "socket/socket_table/socket_table.h"
#include "socket/socket_tree/socket_tree.h"
#include "protocols/base_protocol.h"
#include <rte_lcore.h>

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

ipaddr_t target_ip("10.233.1.1");

dpdk_config_user usrconfig = {
    .lcores = {0},
    .ports = {"0000:01:00.0"},
    .gateway_for_ports = {"90:e2:ba:8a:c7:a1"},
    .queue_num_per_port = {1},
    .always_accurate_time = true,
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

void config_tcp_variables()
{
    TCP::tcp_init();
    TCP::flood = 0;
    TCP::server = 0;
    TCP::send_window = SEND_WINDOW_DEFAULT;
    // TCP::send_window = 0;
    TCP::template_tcp_data = template_tcp_data;
    TCP::template_tcp_opt = template_tcp_opt;
    TCP::template_tcp_pkt = template_tcp_pkt;
    TCP::global_duration_time = 60 * 1000;
    TCP::global_keepalive = false;
    TCP::global_stop = false;
    /* tsc */
    TCP::keepalive_request_interval = 1000;
    TCP::setted_keepalive_request_num = 0; // client
    TCP::global_tcp_rst = true;
    TCP::tos = 0x00;
    TCP::use_http = true;
    TCP::global_mss = MSS_IPV4;
    template_tcp->timer_tsc = 0;
    template_tcp->retrans = 0;
    template_tcp->keepalive_request_num = 0;
    template_tcp->keepalive = TCP::global_keepalive;
    uint32_t seed = (uint32_t)rte_rdtsc();
    template_tcp->snd_nxt = rand_r(&seed);
    template_tcp->snd_una = template_tcp->snd_nxt;
    template_tcp->rcv_nxt = 0;
    template_tcp->state = TCP_CLOSE;
}

void config_http_variables()
{
    HTTP::parser_init();
    HTTP::payload_size = 100;
    HTTP::payload_random = false;
    strcpy(HTTP::http_host, HTTP_HOST_DEFAULT);
    strcpy(HTTP::http_path, HTTP_PATH_DEFAULT);
    http_set_payload(HTTP::payload_size);
    template_http->http_length = 0;
    template_http->http_parse_state = 0;
    template_http->http_flags = 0;
    template_http->http_ack = 0;
}

void config_socket()
{
    template_socket->src_addr = 0;
    template_socket->dst_addr = target_ip;
    template_socket->src_port = 0;
    template_socket->dst_port = 0;
    template_socket->set_protocol(SOCKET_L2, ether);
    template_socket->set_protocol(SOCKET_L3, ip);
    template_socket->set_protocol(SOCKET_L4, template_tcp);
    template_socket->set_protocol(SOCKET_L5, template_http);
}

void config_template_pkt()
{
    template_tcp_pkt->mbuf_pool = mbuf_pool_create(&config, "template_tcp_pkt", config.ports[0].id, 0);
    mbuf_template_pool_setby_socket(template_tcp_pkt, template_socket, nullptr, 0);
    TCP::constructing_opt_tmeplate = true;
    template_tcp_opt->mbuf_pool = mbuf_pool_create(&config, "template_tcp_opt", config.ports[0].id, 0);
    mbuf_template_pool_setby_socket(template_tcp_opt, template_socket, nullptr, 0);
}

int start_test(__rte_unused void *arg1)
{
    ipaddr_t base_src = ipaddr_t("10.233.1.2");
    uint64_t begin_ts = current_ts_msec();
    while (true)
    {
        rte_mbuf *m = nullptr;
        Socket *socket = template_socket;
        TCP* tcp = template_tcp;


        socket->dst_port = rand_() % 20 + 1;
        socket->src_port = rand_() % (65536 - 1024) + 1024;
        socket->src_addr = rand_() % 11 + base_src;

        template_tcp->snd_nxt = rand_();
        // template_tcp->rcv_nxt = 0;
        
        tcp_reply(tcp, socket, TH_SYN);


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
    config_tcp_variables();
    config_http_variables();
    config_socket();
    config_template_pkt();
    rte_eal_remote_launch(start_test, NULL, lcore_id);
    start_test(NULL);
}