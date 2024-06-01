#include "common/primitives.h"
#include <vector>
#include "protocols/global_states.h"
#include "common/ctl.h"

dpdk_config * primitives::dpdk_config_pri;

std::vector<protocol_config> primitives::p_configs;

std::vector<Socket> primitives::sockets_ready_to_add;

// thread_local Socket* primitives::socket_partby_pattern = new Socket[MAX_PATTERNS * MAX_SOCKETS_RANGE_PER_PATTERN];
thread_local Socket* primitives::socket_partby_pattern;

thread_local int primitives::socketsize_partby_pattern[MAX_PATTERNS];

thread_local int primitives::socketpointer_partby_pattern[MAX_PATTERNS];

primitives::random_socket_t primitives::random_methods[MAX_PATTERNS];

uint8_t get_packet_pattern(rte_mbuf *m)
{
    tcphdr* th = rte_pktmbuf_mtod_offset(m, tcphdr*, sizeof(ethhdr) + sizeof(iphdr));
    return th->res1;
}

int get_index(int pattern, int index)
{
    return (pattern * MAX_SOCKETS_RANGE_PER_PATTERN + index);
}

int thread_main(void* arg)
{
#ifdef DEBUG_
    setbuf(stdout, NULL);
#endif
    memset(primitives::socketsize_partby_pattern, 0, sizeof(primitives::socketsize_partby_pattern));
    int max_idx_of_preset = 0;
    for(int i = 0; i < g_pattern_num; i++)
    {
        if(primitives::p_configs[i].preset)
        {
            max_idx_of_preset = std::max(max_idx_of_preset, i+1);
        }
    }
    primitives::socket_partby_pattern = new Socket[max_idx_of_preset * MAX_SOCKETS_RANGE_PER_PATTERN];

    for(int i = 0; i < g_pattern_num ; i++)
    {
        config_protocols(i, &primitives::p_configs[i]);
    }
    for(int i = primitives::sockets_ready_to_add.size()-1; i >= 0; i--)
    {
        Socket socket = primitives::sockets_ready_to_add[i];
        if(!rss_check_socket(&socket))
        {
            continue;
        }
        if(g_vars[socket.pattern].p_type == p_tcp)
        {
            if(g_vars[socket.pattern].tcp_vars.server)
            {
                Socket* ths_socket = tcp_new_socket(&template_socket[socket.pattern]);
                memcpy(ths_socket, &socket, sizeof(FiveTuples));
                ths_socket->protocol = IPPROTO_TCP;
                tcp_validate_csum(ths_socket);
                TCP::socket_table->insert_socket(ths_socket);
            }
            else
            {
                Socket* ths_socket = tcp_new_socket(&template_socket[socket.pattern]);
                memcpy(ths_socket, &socket, sizeof(FiveTuples));
                ths_socket->protocol = IPPROTO_TCP;
                tcp_validate_csum(ths_socket);
                primitives::socket_partby_pattern[get_index(socket.pattern, primitives::socketsize_partby_pattern[socket.pattern])] = *ths_socket;
                primitives::socketsize_partby_pattern[socket.pattern]++;
                delete ths_socket;
            }
        }
        else if(g_vars[socket.pattern].p_type == p_udp)
        {
            Socket* ths_socket = new Socket(template_socket[socket.pattern]);
            memcpy(ths_socket, &socket, sizeof(FiveTuples));
            ths_socket->protocol = IPPROTO_UDP;
            udp_validate_csum(ths_socket);
            primitives::socket_partby_pattern[get_index(socket.pattern, primitives::socketsize_partby_pattern[socket.pattern])] = *ths_socket;
            primitives::socketsize_partby_pattern[socket.pattern]++;
            delete ths_socket;
        }
        else if(g_vars[socket.pattern].p_type == p_tcp_syn)
        {
            Socket* ths_socket = new Socket(template_socket[socket.pattern]);
            memcpy(ths_socket, &socket, sizeof(FiveTuples));
            ths_socket->protocol = IPPROTO_TCP;
            tcp_validate_csum_opt(ths_socket);
            primitives::socket_partby_pattern[get_index(socket.pattern, primitives::socketsize_partby_pattern[socket.pattern])] = *ths_socket;
            primitives::socketsize_partby_pattern[socket.pattern]++;
            delete ths_socket;
        }
        else if(g_vars[socket.pattern].p_type == p_tcp_ack)
        {
            Socket* ths_socket = new Socket(template_socket[socket.pattern]);
            memcpy(ths_socket, &socket, sizeof(FiveTuples));
            ths_socket->protocol = IPPROTO_TCP;
            tcp_validate_csum_pkt(ths_socket);
            primitives::socket_partby_pattern[get_index(socket.pattern, primitives::socketsize_partby_pattern[socket.pattern])] = *ths_socket;
            primitives::socketsize_partby_pattern[socket.pattern]++;
            delete ths_socket;
        }
    }

    primitives::sockets_ready_to_add.resize(0);

    memset(primitives::socketpointer_partby_pattern, 0, sizeof(primitives::socketpointer_partby_pattern));

    g_config_percore->start = true;
    while (g_config_percore->start)
    {
        dpdk_config_percore::enter_epoch();
        int recv_num = 0;
        #pragma unroll
        do
        {
            recv_num++;
            rte_mbuf *m = dpdk_config_percore::cfg_recv_packet();
            dpdk_config_percore::time_update();
            if (!m)
            {
                break;
            }
            uint8_t pattern = get_packet_pattern(m);
            if(g_vars[pattern].p_type == p_tcp){
                Socket* parse_socket = parse_packet(m);
                Socket *socket = TCP::socket_table->find_socket(parse_socket);
                if (socket == nullptr)
                {
                    socket = tcp_new_socket(&template_socket[pattern]);
                    socket->pattern = pattern;
                    memcpy(socket, parse_socket, sizeof(FiveTuples));
                }
                socket->tcp.process(socket, m);
            }
            else if (g_vars[pattern].p_type == p_udp)
            {

            }
            else if (g_vars[pattern].p_type == p_tcp_syn || g_vars[pattern].p_type == p_tcp_ack)
            {

            }
        } while (recv_num < 4);

        dpdk_config_percore::cfg_send_flush();
        
        #pragma unroll
        for (int i = 0; i < MAX_PATTERNS; i++)
        {
            if(i >= g_pattern_num)
                break;
            if(g_vars[i].p_type == p_tcp)
            {
                if(g_vars[i].tcp_vars.use_http) http_ack_delay_flush(i);
                
                if (g_vars[i].tcp_vars.server)
                {
                    continue;
                }
                else
                {
                    int fail_cnt = 0;
                    int launch_num = dpdk_config_percore::check_epoch_timer(i);
                    if(g_vars[i].tcp_vars.preset)
                    {
                        for (int j = 0; j < launch_num; j++)
                        {
                            dpdk_config_percore::time_update();
repick:
                            Socket *socket = &primitives::socket_partby_pattern[get_index(i, primitives::socketpointer_partby_pattern[i])];
                            primitives::socketpointer_partby_pattern[i]++;
                            
                            if (unlikely(primitives::socketpointer_partby_pattern[i] >= primitives::socketsize_partby_pattern[i]))
                            {
                                primitives::socketpointer_partby_pattern[i] = 0;
                            }

                            if (TCP::socket_table->insert_socket(socket) == -1)
                            {
                                if(unlikely(fail_cnt >= RERAND_MAX_NUM))
                                {
                                    goto continue_epoch;
                                }
                                fail_cnt++;
                                goto repick;
                            }
                            tcp_launch(socket);
                        }
                    }
                    else
                    {
                        for (int j = 0; j < launch_num; j++)
                        {
                            dpdk_config_percore::time_update();
                            Socket *socket = tcp_new_socket(&template_socket[i]);
rerand:
                            primitives::random_methods[i](socket);

                            if (!rss_check_socket(socket) || TCP::socket_table->insert_socket(socket) == -1)
                            {
                                if(unlikely(fail_cnt >= RERAND_MAX_NUM))
                                {
                                    goto continue_epoch;
                                }
                                fail_cnt++;
                                goto rerand;
                            }
                            tcp_validate_csum(socket);
                            tcp_launch(socket);
                        }
                    }
                }
            }
            else if(g_vars[i].p_type == p_udp)
            {
                int launch_num = dpdk_config_percore::check_epoch_timer(i);
                if(g_vars[i].udp_vars.preset)
                {
                    for (int j = 0; j < launch_num; j++)
                    {
                        Socket *socket = &primitives::socket_partby_pattern[get_index(i, primitives::socketpointer_partby_pattern[i])];
                        primitives::socketpointer_partby_pattern[i]++;
                        if (unlikely(primitives::socketpointer_partby_pattern[i] >= primitives::socketsize_partby_pattern[i]))
                        {
                            primitives::socketpointer_partby_pattern[i] = 0;
                        }
                        udp_send(socket);
                    }
                }
                else
                {
                    for (int j = 0; j < launch_num; j++)
                    {
                        primitives::random_methods[i](&template_socket[i]);
                        udp_validate_csum(&template_socket[i]);
                        udp_send(&template_socket[i]);
                    }
                }
                
            }
            else if (g_vars[i].p_type == p_tcp_syn)
            {
                int launch_num = dpdk_config_percore::check_epoch_timer(i);
                if(g_vars[i].tcp_vars.preset)
                {
                    for (int j = 0; j < launch_num; j++)
                    {
                        Socket *socket = &primitives::socket_partby_pattern[get_index(i, primitives::socketpointer_partby_pattern[i])];
                        primitives::socketpointer_partby_pattern[i]++;
                        if (unlikely(primitives::socketpointer_partby_pattern[i] >= primitives::socketsize_partby_pattern[i]))
                        {
                            primitives::socketpointer_partby_pattern[i] = 0;
                        }
                        tcp_reply(socket, TH_SYN);
                    }
                }
                else
                {
                    for (int j = 0; j < launch_num; j++)
                    {
                        primitives::random_methods[i](&template_socket[i]);
                        tcp_validate_csum_opt(&template_socket[i]);
                        tcp_reply(&template_socket[i], TH_SYN);
                    }
                }
            }
            else if (g_vars[i].p_type == p_tcp_ack)
            {
                int launch_num = dpdk_config_percore::check_epoch_timer(i);
                if(g_vars[i].tcp_vars.preset)
                {
                    for (int j = 0; j < launch_num; j++)
                    {
                        Socket *socket = &primitives::socket_partby_pattern[get_index(i, primitives::socketpointer_partby_pattern[i])];
                        primitives::socketpointer_partby_pattern[i]++;
                        if (unlikely(primitives::socketpointer_partby_pattern[i] >= primitives::socketsize_partby_pattern[i]))
                        {
                            primitives::socketpointer_partby_pattern[i] = 0;
                        }
                        tcp_reply(socket, TH_ACK);
                    }
                }
                else
                {
                    for (int j = 0; j < launch_num; j++)
                    {
                        primitives::random_methods[i](&template_socket[i]);
                        tcp_validate_csum_pkt(&template_socket[i]);
                        tcp_reply(&template_socket[i], TH_ACK);
                    }
                }
            }
        }
continue_epoch:
        TIMERS.trigger();
    }
    return 0;
}

void primitives::set_configs_and_init(dpdk_config_user& usrconfig, char** argv)
{
    dpdk_config_pri = new dpdk_config(&usrconfig);
    dpdk_init(dpdk_config_pri, argv[0]);
}

void primitives::set_pattern_num(int pattern_num)
{
    g_pattern_num = pattern_num;
}

void primitives::add_pattern(protocol_config& p_config)
{
    int index = p_configs.size();

    template_socket[index].src_addr = p_config.template_ip_src;
    template_socket[index].dst_addr = p_config.template_ip_dst;
    template_socket[index].src_port = atoi(p_config.template_port_src.data());
    template_socket[index].dst_port = atoi(p_config.template_port_dst.data());
    if(p_config.protocol == "TCP")
    {
        template_socket[index].protocol = IPPROTO_TCP;
        g_vars[index].p_type = p_tcp;
    }
    else if(p_config.protocol == "UDP")
    {
        template_socket[index].protocol = IPPROTO_UDP;
        g_vars[index].p_type = p_udp;
    }
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

void primitives::run_generate()
{
#ifdef USE_CTL_THREAD
    pthread_t thread;

    ctl_thread_start(g_config, &thread);
#endif

    if(g_config->num_lcores == 1)
    {
        thread_main(nullptr);
    }
    else
    {
        dpdk_run(thread_main, NULL);
    }

#ifdef USE_CTL_THREAD
    ctl_thread_wait(thread);
#endif
}