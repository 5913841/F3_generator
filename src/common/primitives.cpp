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

primitives::update_speed_t primitives::update_speed_methods[MAX_PATTERNS];

uint8_t pattern_order[MAX_PATTERNS];

void* rand_data[MAX_PATTERNS];

void* update_speed_data[MAX_PATTERNS];

Socket* five_tuples_pointer[MAX_PATTERNS];
FTRange* five_tuples_range_pointer[MAX_PATTERNS];

void update_speed_default(launch_control* lc, void* data)
{
    return;
}

struct slow_start_data
{
    int pattern;
    int second_prev;
    int step;
};

void update_speed_slow_start(launch_control* lc, void* data)
{
    slow_start_data* ssd = (slow_start_data*)data;
    int pattern = ssd->pattern;
    int second_prev = ssd->second_prev;
    int slow_start = g_vars[pattern].slow_start;
    int second_now = second_in_config();
    if(second_now <= slow_start && slow_start != 0 && second_now > second_prev){
        int launch_num = g_vars[pattern].launch_num;
        // int step = (g_vars[pattern].cps / g_config->num_lcores) / slow_start;
        int step = ssd->step;
        if (step <= 0) {
            step = 1;
        }
        int cps = step * second_now;
        uint64_t launch_interval;
        if(cps == 0) launch_interval = g_tsc_per_second * (1 << 14);
        else launch_interval = (g_tsc_per_second * launch_num) / cps;
        lc->launch_last = time_in_config();
        lc->launch_interval = launch_interval;
        second_prev = second_now;
    }
}

uint8_t get_packet_pattern(rte_mbuf *m)
{
    tcphdr* th = rte_pktmbuf_mtod_offset(m, tcphdr*, sizeof(ethhdr) + sizeof(iphdr));
    return th->res1;
}

const uint8_t& get_packet_l4_protocol(rte_mbuf *m)
{
    // iphdr* ip = rte_pktmbuf_mtod_offset(m, iphdr*, sizeof(ethhdr));
    if(rte_pktmbuf_mtod_offset(m, const uint8_t*, sizeof(ethhdr))[0] & 0x0f == 0x0f)
    {
        return rte_pktmbuf_mtod_offset(m, iphdr*, sizeof(ethhdr))->protocol;
    }
    else
    {
        return rte_pktmbuf_mtod_offset(m, ip6_hdr*, sizeof(ethhdr))->ip6_ctlun.ip6_un1.ip6_un1_nxt;
    }
}

int min_idx_of_preset = INT_MAX;

int primitives::get_index(int pattern, int index)
{
    return ((pattern - min_idx_of_preset) * MAX_SOCKETS_RANGE_PER_PATTERN + index);
}

void check_and_reoder_pattern()
{
    for(int i = 0; i < g_pattern_num; i++)
    {
        pattern_order[i] = i;
    }
    if(primitives::p_configs[0].protocol != "TCP")
    {
        return;
    }
    for(int i = 1; i < g_pattern_num; i++)
    {
        if (primitives::p_configs[i].protocol != "TCP")
        {
            pattern_order[0] = i;
            pattern_order[i] = 0;
            std::swap(primitives::p_configs[0], primitives::p_configs[i]);
            std::swap(primitives::random_methods[0], primitives::random_methods[i]);
            std::swap(primitives::update_speed_methods[0], primitives::update_speed_methods[i]);
            for(int i = 0; i < primitives::sockets_ready_to_add.size(); i++)
            {
                Socket& socket = primitives::sockets_ready_to_add[i];
                socket.pattern = pattern_order[socket.pattern];
            }
            break;
        }
    }
}

int thread_main(void* arg)
{
#ifdef DEBUG_
    setbuf(stdout, NULL);
#endif
    check_and_reoder_pattern();
    memset(primitives::socketsize_partby_pattern, 0, sizeof(primitives::socketsize_partby_pattern));
    int max_idx_of_preset = 0;
    for(int i = 0; i < g_pattern_num; i++)
    {
        if(primitives::p_configs[i].preset)
        {
            min_idx_of_preset = std::min(min_idx_of_preset, i);
            max_idx_of_preset = std::max(max_idx_of_preset, i+1);
        }
    }
    
    if(min_idx_of_preset != INT_MAX) primitives::socket_partby_pattern = new Socket[(max_idx_of_preset-min_idx_of_preset) * MAX_SOCKETS_RANGE_PER_PATTERN];

    for(int i = 0; i < g_pattern_num ; i++)
    {
        config_protocols(i, &primitives::p_configs[i]);
    }

    for(int i = 0; i < g_pattern_num; i++)
    {
        if (primitives::update_speed_methods[i] == NULL)
        {
            if (g_vars[i].slow_start != 0 && (g_vars[i].p_type != p_tcp || !g_vars[i].tcp_vars.server))
            {
                primitives::update_speed_methods[i] = update_speed_slow_start;
                slow_start_data* ssd = new slow_start_data;
                ssd->pattern = i;
                ssd->second_prev = 0;
                ssd->step = (g_vars[i].cps / g_config->num_lcores) / g_vars[i].slow_start;
                update_speed_data[i] = ssd;
            }
            else
                primitives::update_speed_methods[i] = update_speed_default;
        }
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
            if(g_vars[socket.pattern].tcp_vars.server || !g_vars[socket.pattern].tcp_vars.use_flowtable)
            {
                Socket* ths_socket = tcp_new_socket(&template_socket[socket.pattern]);
                memcpy(ths_socket, &socket, sizeof(FiveTuples));
                ths_socket->protocol = IPPROTO_TCP;
                tcp_validate_csum(ths_socket);
                tcp_insert_socket(ths_socket, ths_socket->pattern);
            }
            else
            {
                Socket* ths_socket = tcp_new_socket(&template_socket[socket.pattern]);
                memcpy(ths_socket, &socket, sizeof(FiveTuples));
                ths_socket->protocol = IPPROTO_TCP;
                tcp_validate_csum(ths_socket);
                primitives::socket_partby_pattern[primitives::get_index(socket.pattern, primitives::socketsize_partby_pattern[socket.pattern])] = *ths_socket;
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
            primitives::socket_partby_pattern[primitives::get_index(socket.pattern, primitives::socketsize_partby_pattern[socket.pattern])] = *ths_socket;
            primitives::socketsize_partby_pattern[socket.pattern]++;
            delete ths_socket;
        }
        else if(g_vars[socket.pattern].p_type == p_tcp_syn)
        {
            Socket* ths_socket = new Socket(template_socket[socket.pattern]);
            memcpy(ths_socket, &socket, sizeof(FiveTuples));
            ths_socket->protocol = IPPROTO_TCP;
            tcp_validate_csum_opt(ths_socket);
            primitives::socket_partby_pattern[primitives::get_index(socket.pattern, primitives::socketsize_partby_pattern[socket.pattern])] = *ths_socket;
            primitives::socketsize_partby_pattern[socket.pattern]++;
            delete ths_socket;
        }
        else if(g_vars[socket.pattern].p_type == p_tcp_ack)
        {
            Socket* ths_socket = new Socket(template_socket[socket.pattern]);
            memcpy(ths_socket, &socket, sizeof(FiveTuples));
            ths_socket->protocol = IPPROTO_TCP;
            tcp_validate_csum_pkt(ths_socket);
            primitives::socket_partby_pattern[primitives::get_index(socket.pattern, primitives::socketsize_partby_pattern[socket.pattern])] = *ths_socket;
            primitives::socketsize_partby_pattern[socket.pattern]++;
            delete ths_socket;
        }
    }

    primitives::sockets_ready_to_add.resize(0);

    memset(primitives::socketpointer_partby_pattern, 0, sizeof(primitives::socketpointer_partby_pattern));
    for(int i = 0; i < g_pattern_num; i++)
    {
        if(!g_vars[i].tcp_vars.use_flowtable)
        {
            if(g_vars[i].tcp_vars.preset)
            {
                five_tuples_range_pointer[i] = &g_vars[i].tcp_vars.socket_range_table->range;
                set_start_ft(five_tuples_pointer[i], *five_tuples_range_pointer[i]);
            }
            else
            {
                five_tuples_range_pointer[i] = &g_vars[i].tcp_vars.socket_pointer_range_table->range;
                set_start_ft(five_tuples_pointer[i], *five_tuples_range_pointer[i]);
            }
        }
    }

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
            const uint8_t& l4_protocol = get_packet_l4_protocol(m);
            uint8_t pattern = get_packet_pattern(m);
            if(l4_protocol == IPPROTO_TCP)
            {
                if(g_vars[pattern].p_type == p_tcp){
                    Socket* parse_socket = parse_packet(m);
                    Socket *socket = tcp_find_socket(parse_socket, pattern);
                    if (socket == nullptr)
                    {
                        socket = tcp_new_socket(&template_socket[pattern]);
                        socket->pattern = pattern;
                        memcpy(socket, parse_socket, sizeof(FiveTuples));
                    }
                    socket->tcp.process(socket, m);
                }
                else if (g_vars[pattern].p_type == p_tcp_syn)
                {
                    mbuf_free2(m);
                    net_stats_tcp_drop();
                    net_stats_tcp_rx();
                    net_stats_syn_rx();
                }
                else if (g_vars[pattern].p_type == p_tcp_ack)
                {
                    mbuf_free2(m);
                    net_stats_tcp_drop();
                    net_stats_tcp_rx();
                }
            }
            else if(l4_protocol == IPPROTO_UDP)
            {
                mbuf_free2(m);
                net_stats_udp_drop();
                net_stats_udp_rx();
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
                    primitives::update_speed_methods[i](&g_config_percore->launch_ctls[i], update_speed_data[i]);
                    int launch_num = dpdk_config_percore::check_epoch_timer(i);
                    if(g_vars[i].tcp_vars.preset)
                    {
                        if(g_vars[i].tcp_vars.use_flowtable)
                        {
                            for (int j = 0; j < launch_num; j++)
                            {
                                dpdk_config_percore::time_update();
repick:
                                Socket *socket = &primitives::socket_partby_pattern[primitives::get_index(i, primitives::socketpointer_partby_pattern[i])];
                                primitives::socketpointer_partby_pattern[i]++;
                                
                                if (unlikely(primitives::socketpointer_partby_pattern[i] >= primitives::socketsize_partby_pattern[i]))
                                {
                                    primitives::socketpointer_partby_pattern[i] = 0;
                                }

                                if (tcp_insert_socket(socket, i) == -1)
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
repick_noft:
                                Socket *socket = five_tuples_pointer[i];
                                increase_ft(five_tuples_pointer[i], *five_tuples_range_pointer[i]);
                                if (unlikely(primitives::socketpointer_partby_pattern[i] >= primitives::socketsize_partby_pattern[i]))
                                {
                                    primitives::socketpointer_partby_pattern[i] = 0;
                                }

                                if (tcp_insert_socket(socket, i) == -1)
                                {
                                    if(unlikely(fail_cnt >= RERAND_MAX_NUM))
                                    {
                                        goto continue_epoch;
                                    }
                                    fail_cnt++;
                                    goto repick_noft;
                                }
                                tcp_launch(socket);
                            }
                        }
                    }
                    else
                    {
                        for (int j = 0; j < launch_num; j++)
                        {
                            dpdk_config_percore::time_update();
                            Socket *socket = tcp_new_socket(&template_socket[i]);
rerand:
                            primitives::random_methods[i](socket, rand_data[i]);

                            if (!rss_check_socket(socket) || tcp_insert_socket(socket, i) == -1)
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
                primitives::update_speed_methods[i](&g_config_percore->launch_ctls[i], update_speed_data[i]);
                int launch_num = dpdk_config_percore::check_epoch_timer(i);
                if(g_vars[i].udp_vars.preset)
                {
                    for (int j = 0; j < launch_num; j++)
                    {
                        Socket *socket = &primitives::socket_partby_pattern[primitives::get_index(i, primitives::socketpointer_partby_pattern[i])];
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
                        primitives::random_methods[i](&template_socket[i], rand_data[i]);
                        udp_validate_csum(&template_socket[i]);
                        udp_send(&template_socket[i]);
                    }
                }
                
            }
            else if (g_vars[i].p_type == p_tcp_syn)
            {
                primitives::update_speed_methods[i](&g_config_percore->launch_ctls[i], update_speed_data[i]);
                int launch_num = dpdk_config_percore::check_epoch_timer(i);
                if(g_vars[i].tcp_vars.preset)
                {
                    for (int j = 0; j < launch_num; j++)
                    {
                        Socket *socket = &primitives::socket_partby_pattern[primitives::get_index(i, primitives::socketpointer_partby_pattern[i])];
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
                        primitives::random_methods[i](&template_socket[i], rand_data[i]);
                        tcp_validate_csum_opt(&template_socket[i]);
                        tcp_reply(&template_socket[i], TH_SYN);
                    }
                }
            }
            else if (g_vars[i].p_type == p_tcp_ack)
            {
                primitives::update_speed_methods[i](&g_config_percore->launch_ctls[i], update_speed_data[i]);
                int launch_num = dpdk_config_percore::check_epoch_timer(i);
                if(g_vars[i].tcp_vars.preset)
                {
                    for (int j = 0; j < launch_num; j++)
                    {
                        Socket *socket = &primitives::socket_partby_pattern[primitives::get_index(i, primitives::socketpointer_partby_pattern[i])];
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
                        primitives::random_methods[i](&template_socket[i], rand_data[i]);
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
void primitives::add_fivetuples(const FiveTuples& five_tuples, int pattern)
{
    sockets_ready_to_add.push_back(Socket(five_tuples, pattern));
}

void primitives::add_fivetuples(const Socket& socket, int pattern)
{
    sockets_ready_to_add.push_back(socket);
    sockets_ready_to_add.back().pattern = pattern;
}

//should be use in non-preset mode
void primitives::set_random_method(random_socket_t random_method, int pattern_num, void* data)
{
    random_methods[pattern_num] = random_method;
    rand_data[pattern_num] = data;
}

void primitives::set_update_speed_method(update_speed_t update_speed_method, int pattern_num, void* data)
{
    update_speed_methods[pattern_num] = update_speed_method;
    update_speed_data[pattern_num] = data;
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