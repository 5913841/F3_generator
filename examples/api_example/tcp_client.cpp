#include "api/api.h"
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)


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
    while (true)
    {
        api::api_enter_epoch();
        do
        {
            rte_mbuf *m = api::api_recv_packet();
            api::api_time_update();
            if (!m)
            {
                break;
            }
            Socket* parse_socket = api::api_parse_packet(m);
            Socket *socket = api::api_flowtable_find_socket(parse_socket);
            if (socket != nullptr)
                api::api_process_tcp(socket, m);
        } while (true);
        api::api_send_flush();

        int launch_num = api::api_check_timer(0);
        for (int i = 0; i < launch_num; i++)
        {
            api::api_time_update();
            Socket *socket = api::api_tcp_new_socket(template_socket);

            socket->dst_port = rand() % 20 + 1;
            socket->src_port = rand();
            socket->src_addr = rand() % 11 + base_src;
            if (api::api_flowtable_insert_socket(socket) == -1)
            {
                api::api_tcp_release_socket(socket);
                continue;
            }
            api::api_tcp_validate_csum(socket);
            api::api_tcp_launch(socket);
        }
        api::api_trigger_timers();
    }
    return 0;
}

int main(int argc, char **argv)
{
    api::api_set_configs_and_init(usrconfig, argv);
    api::api_set_pattern_num(1);
    api::api_config_protocols(0, &p_config);
    start_test(NULL);
}