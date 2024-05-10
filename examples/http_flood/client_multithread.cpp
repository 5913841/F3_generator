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
// #include "socket/socket_table/parallel_socket_table.h"
#include "socket/socket_table/socket_table.h"
#include "socket/socket_tree/socket_tree.h"
#include "protocols/base_protocol.h"
#include "dpdk/divert/rss.h"
#include <rte_lcore.h>

// #define RTE_PKTMBUF_HEADROOM 256
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

thread_local ETHER *ether = new ETHER();
thread_local IPV4 *ip = new IPV4();
thread_local TCP *template_tcp = new TCP();
thread_local HTTP *template_http = new HTTP();
thread_local Socket *template_socket = new Socket();
thread_local mbuf_cache *template_tcp_data = new mbuf_cache();
thread_local mbuf_cache *template_tcp_opt = new mbuf_cache();
thread_local mbuf_cache *template_tcp_pkt = new mbuf_cache();
thread_local const char *data;
thread_local SocketPointerTable *socket_table = new SocketPointerTable();

ip4addr_t target_ip("10.233.1.1");

dpdk_config_user usrconfig = {
    .lcores = {0, 1},
    .ports = {"0000:01:00.1"},
    .gateway_for_ports = {"90:e2:ba:87:62:98"},
    .queue_num_per_port = {2},
    .always_accurate_time = false,
    .flow_distribution_strategy = "rss",
    .rss_type = "l3",
    .mq_rx_rss = true,
    .tx_burst_size = 8,
    .rx_burst_size = 8,
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
    TCP::keepalive_request_interval = 1000;
    TCP::tcp_init();
    TCP::flood = 0;
    TCP::server = 0;
    // TCP::send_window = SEND_WINDOW_DEFAULT;
    TCP::send_window = 0;
    TCP::template_tcp_data = template_tcp_data;
    TCP::template_tcp_opt = template_tcp_opt;
    TCP::template_tcp_pkt = template_tcp_pkt;
    TCP::global_duration_time = 60 * 1000;
    TCP::global_keepalive = false;
    TCP::global_stop = false;
    /* tsc */
    TCP::setted_keepalive_request_num = 0; // client
    TCP::release_socket_callback = [](Socket *sk)
    { socket_table->remove_socket(sk); tcp_release_socket(sk); };
    TCP::checkvalid_socket_callback = [](FiveTuples ft, Socket *sk)
    { return socket_table->find_socket(ft) == sk; };
    TCP::global_tcp_rst = true;
    TCP::tos = 0x00;
    TCP::use_http = true;
    TCP::global_mss = MSS_IPV4;
    data = TCP::server ? http_get_response() : http_get_request();
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
    template_tcp_data->mbuf_pool = mbuf_pool_create(&config, (std::string("template_tcp_data") + std::to_string(g_config_percore->lcore_id)).c_str(), g_config_percore->port_id, g_config_percore->queue_id);
    mbuf_template_pool_setby_socket(template_tcp_data, template_socket, data, strlen(data));
    template_tcp_pkt->mbuf_pool = mbuf_pool_create(&config, (std::string("template_tcp_pkt") + std::to_string(g_config_percore->lcore_id)).c_str(), g_config_percore->port_id, g_config_percore->queue_id);
    mbuf_template_pool_setby_socket(template_tcp_pkt, template_socket, nullptr, 0);
    TCP::constructing_opt_tmeplate = true;
    template_tcp_opt->mbuf_pool = mbuf_pool_create(&config, (std::string("template_tcp_opt") + std::to_string(g_config_percore->lcore_id)).c_str(), g_config_percore->port_id, g_config_percore->queue_id);
    mbuf_template_pool_setby_socket(template_tcp_opt, template_socket, nullptr, 0);
}

int start_test(__rte_unused void *arg1)
{
    ip4addr_t base_src = ip4addr_t("10.233.1.2");
    uint64_t begin_ts = current_ts_msec();
    Socket *parser_socket = new Socket();
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
            parse_packet(m, parser_socket);
            Socket *socket = socket_table->find_socket(parser_socket);
            if (socket != nullptr)
                socket->l4_protocol->process(socket, m);
        } while (true);
        dpdk_config_percore::cfg_send_flush();
        http_ack_delay_flush();

        // if (current_ts_msec() - begin_ts > 10 * 1000)
        // {
        //     break;
        // }

        if (dpdk_config_percore::check_epoch_timer(0.000007 * TSC_PER_SEC))
        {
            for (int i = 0; i < 7; i++)
            {
                Socket *socket = tcp_new_socket(template_socket);
rerand:
                socket->dst_port = rand() % 20 + 1;
                socket->src_port = rand();
                socket->src_addr = rand() % 11 + base_src;
                if (!rss_check_socket(socket) || socket_table->insert_socket(socket) == -1)
                {
                    goto rerand;
                }
                tcp_validate_csum(socket);
                tcp_launch(socket);
            }
            TIMERS.trigger();
        }
    }
    delete parser_socket;
    return 0;
}

int thread_main(void *arg)
{
    config_ether_variables();
    config_ip_variables();
    config_tcp_variables();
    config_http_variables();
    config_socket();
    config_template_pkt();
    start_test(NULL);
    return 0;
}

int main(int argc, char **argv)
{
    dpdk_init(&config, argv[0]);
    dpdk_run(thread_main, NULL);
    return 0;
}