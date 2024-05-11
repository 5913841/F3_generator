#include "common/primitives.h"

using namespace primitives;

dpdk_config primitives::dpdk_config_pri;

std::vector<protocol_config> primitives::p_configs;

std::vector<Socket> primitives::sockets_ready_to_add;

thread_local Socket primitives::socket_partby_pattern[MAX_PATTERNS][MAX_SOCKETS_RANGE_PER_PATTERN];

thread_local int primitives::socketsize_partby_pattern[MAX_PATTERNS];

thread_local int primitives::socketpointer_partby_pattern[MAX_PATTERNS];

random_socket_t primitives::random_methods[MAX_PATTERNS];

static int thread_main(void* arg)
{
    for(int i = 0; i < TCP::pattern_num ; i++)
    {
        config_protocols(i, &p_configs[i]);
    }
    delete &p_configs;
    for(int i = 0; i < sockets_ready_to_add.size(); i++)
    {
        Socket socket = sockets_ready_to_add[i];
        if(TCP::g_vars[socket.pattern].server)
        {
            Socket* ths_socket = tcp_new_socket(&template_socket[socket.pattern]);
            memcpy(ths_socket, &socket, sizeof(FiveTuples));
            TCP::socket_table->insert_socket(ths_socket);
        }
        else
        {
            Socket* ths_socket = tcp_new_socket(&template_socket[socket.pattern]);
            memcpy(ths_socket, &socket, sizeof(FiveTuples));
            socket_partby_pattern[socket.pattern][socketsize_partby_pattern[socket.pattern]] = *ths_socket;
            socketsize_partby_pattern[socket.pattern]++;
        }
    }
    delete &sockets_ready_to_add;

    memset(socketpointer_partby_pattern, 0, sizeof(socketpointer_partby_pattern));

    while (true)
    {
        dpdk_config_percore::enter_epoch();
        int recv_num = 0;
        
        do
        {
            rte_mbuf *m = dpdk_config_percore::cfg_recv_packet();
            dpdk_config_percore::time_update();
            if (!m)
            {
                break;
            }
            recv_num++;
            Socket* parse_socket = parse_packet(m);
            Socket *socket = TCP::socket_table->find_socket(parse_socket);
            if (socket == nullptr)
            {
                socket = tcp_new_socket(template_socket);
                socket->pattern = 0;
                memcpy(socket, parse_socket, sizeof(FiveTuples));
            }
            socket->tcp.process(socket, m);
        } while (recv_num < 4);

        dpdk_config_percore::cfg_send_flush();
        for (int i = 0; i < MAX_PATTERNS; i++)
        {
            if(i >= TCP::pattern_num)
                break;
            
            if(TCP::g_vars[i].use_http) http_ack_delay_flush(i);
            
            if (TCP::g_vars[i].server)
            {
                continue;
            }
            else
            {
                int launch_num = dpdk_config_percore::check_epoch_timer(i);
                if(TCP::g_vars[i].preset)
                {
                    for (int j = 0; j < launch_num; j++)
                    {
                        dpdk_config_percore::time_update();

                        Socket *socket = &socket_partby_pattern[i][socketpointer_partby_pattern[i]];
                        socketpointer_partby_pattern[i]++;
                        
                        if (unlikely(socketpointer_partby_pattern[i] >= socketsize_partby_pattern[i]))
                        {
                            socketpointer_partby_pattern[i] = 0;
                        }

                        if (TCP::socket_table->insert_socket(socket) == -1)
                        {
                            tcp_release_socket(socket);
                            continue;
                        }
                        tcp_validate_csum(socket);
                        tcp_launch(socket);
                    }
                }
                else
                {
                    for (int j = 0; j < launch_num; j++)
                    {
                        dpdk_config_percore::time_update();
                        Socket *socket = tcp_new_socket(&template_socket[i]);
                        random_methods[i](socket);

                        if (TCP::socket_table->insert_socket(socket) == -1)
                        {
                            tcp_release_socket(socket);
                            continue;
                        }
                        tcp_validate_csum(socket);
                        tcp_launch(socket);
                    }
                }
            }
        }
        TIMERS.trigger();
    }
}

void primitives::set_configs_and_init(dpdk_config_user& usrconfig, char** argv)
{
    dpdk_config_pri = dpdk_config(&usrconfig);
    dpdk_init(&dpdk_config_pri, argv[0]);
}

void primitives::set_pattern_num(int pattern_num)
{
    TCP::pattern_num = pattern_num;
    HTTP::pattern_num = pattern_num;
}

void primitives::add_pattern(protocol_config& p_config)
{
    p_configs.push_back(p_config);
}

//should be use in preset mode
void primitives::add_fivetuples(FiveTuples& five_tuples, int pattern)
{
    sockets_ready_to_add.push_back(Socket(five_tuples, pattern));
}

//should be use in non-preset mode
void primitives::set_random_method(random_socket_t random_method, int pattern_num)
{
    random_methods[pattern_num] = random_method;
}

void primitives::set_total_time(std::string total_time)
{

}

void primitives::run_setted()
{
    if(g_config->num_lcores == 1)
    {
        thread_main(nullptr);
    }
    else
    {
        dpdk_run(thread_main, NULL);
    }
}