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
#include "socket/socket.h"
#include "protocols/base_protocol.h"
#include <rte_lcore.h>

// #define RTE_PKTMBUF_HEADROOM 256
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

typedef Socket<ETHER, IPV4, TCP, HTTP> Socket_t;
typedef SocketPointerTable<Socket_t> SocketPointerTable_t;

ETHER *ether = new ETHER();
IPV4 *ip = new IPV4();
TCP *template_tcp = new TCP();
HTTP *template_http = new HTTP();
Socket_t *template_socket = new Socket_t();
mbuf_cache *template_tcp_data = new mbuf_cache();
mbuf_cache *template_tcp_opt = new mbuf_cache();
mbuf_cache *template_tcp_pkt = new mbuf_cache();
const char *data;
SocketPointerTable_t *socket_table = new SocketPointerTable_t();

ipaddr_t target_ip("10.233.1.1");

rx_queue *rq = new rx_queue();

dpdk_config_user usrconfig = {
    .lcores = {0},
    .ports = {"0000:01:00.0"},
    .gateway_for_ports = {"90:e2:ba:8a:c7:a1"},
    .queue_num_per_port = {1},

};

dpdk_config config = dpdk_config(&usrconfig);

void config_ether_variables()
{
    ETHER::parser_init<Socket_t>();
    ether->port = config.ports + 0;
    ether->queue_id = 0;
    ether->tq = new tx_queue();
    ether->tq->tx_burst = TX_BURST_DEFAULT;
    rq->rx_burst = RX_BURST_MAX;
}

void config_ip_variables()
{
    IPV4::parser_init<Socket_t>();
    return;
}

void config_tcp_variables()
{
    TCP::parser_init<Socket_t>();
    TCP::timer_init<Socket_t>();
    TCP::flood = 0;
    TCP::server = 0;
    // TCP::send_window = SEND_WINDOW_DEFAULT;
    TCP::send_window = 0;
    TCP::template_tcp_data = template_tcp_data;
    TCP::template_tcp_opt = template_tcp_opt;
    TCP::template_tcp_pkt = template_tcp_pkt;
    TCP::global_duration_time = 60 * 1000;
    TCP::global_keepalive = true;
    TCP::global_stop = false;
    /* tsc */
    TCP::keepalive_request_interval = 1000;
    TCP::setted_keepalive_request_num = 0; // client
    TCP::release_socket_callback = [](Socket *sk)
    { socket_table->remove_socket(sk); tcp_release_socket(sk); };
    TCP::global_tcp_rst = true;
    TCP::tos = 0x00;
    TCP::use_http = true;
    TCP::global_mss = MSS_IPV4;
    data = TCP::server ? http_get_response() : http_get_request();
    template_tcp->timer_tsc = current_ts_msec();
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
    HTTP::parser_init<Socket_t>();
    HTTP::payload_size = 64;
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
    template_socket->l2_protocol = *ether;
    template_socket->l3_protocol = *ip;
    template_socket->l4_protocol = *template_tcp;
    template_socket->l5_protocol = *template_http;
}

void config_template_pkt()
{
    template_tcp_data->mbuf_pool = mbuf_pool_create(&config, "template_tcp_data", config.ports[0].id, 0);
    mbuf_template_pool_setby_socket(template_tcp_data, template_socket, data, strlen(data));
    template_tcp_pkt->mbuf_pool = mbuf_pool_create(&config, "template_tcp_pkt", config.ports[0].id, 0);
    mbuf_template_pool_setby_socket(template_tcp_pkt, template_socket, nullptr, 0);
    TCP::constructing_opt_tmeplate = true;
    template_tcp_opt->mbuf_pool = mbuf_pool_create(&config, "template_tcp_opt", config.ports[0].id, 0);
    mbuf_template_pool_setby_socket(template_tcp_opt, template_socket, nullptr, 0);
}

int start_test(__rte_unused void *arg1)
{

    uint64_t last_tsc = current_ts_msec();
    ipaddr_t base_src = ipaddr_t("10.233.1.2");
    uint64_t begin_ts = current_ts_msec();
    while (true)
    {
        rte_mbuf *m = nullptr;
        do{
            m = recv_packet_by_queue(rq, config.ports[0].id, 0);
            if (m != nullptr){
                Socket_t* parser_socket = new Socket_t();
                parse_packet(m, parser_socket);
                Socket_t* socket = socket_table->find_socket(parser_socket);
                socket->l4_protocol->process(socket, m);
            }
        } while(m!= nullptr);
        tx_queue_flush(ether->tq, config.ports[0].id, 0);

        if (current_ts_msec() - last_tsc > 1000) {
            last_tsc = current_ts_msec();
            if(last_tsc - begin_ts > 60 * 1000) {
                break;
            }
            for(int i = 0; i < 100000; i++){
                Socket_t* socket = tcp_new_socket(template_socket);

                socket->dst_port = rand() % 20 + 1;
                socket->src_port = rand();
                socket->src_addr = rand() % 11 + base_src;
                if(socket_table->insert_socket(socket) == -1){
                    delete socket;
                    continue;
                }
                tcp_launch(socket);
            }
            TIMERS.trigger();
        }
    }
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
    start_test(NULL);
}