#include "dpdk/divert/rss.h"
#include "protocols/conf_protocol.h"
#include "protocols/global_states.h"

namespace api{

    typedef phmap::parallel_flat_hash_set<Socket *, SocketPointerHash, SocketPointerEqual> socket_set_t;



    socket_set_t& api_get_flowtable()
    {
        return TCP::socket_table->socket_table;
    }

    uint8_t api_get_syn_pattern_by_res(rte_mbuf *mbuf)
    {
        tcphdr* th = rte_pktmbuf_mtod_offset(mbuf, tcphdr*, sizeof(ethhdr) + sizeof(iphdr));
        return th->res1;
    }

    uint8_t api_get_packet_l4_protocol(rte_mbuf *m)
    {
        iphdr* ip = rte_pktmbuf_mtod_offset(m, iphdr*, sizeof(ethhdr));
        if(ip->version == 4)
            return ip->protocol;
        else
        {
            ip6_hdr* ip6 = rte_pktmbuf_mtod_offset(m, ip6_hdr*, sizeof(ethhdr));
            return ip6->ip6_ctlun.ip6_un1.ip6_un1_nxt;
        }
    }

    bool api_socket_thscore(Socket *sk)
    {
        return rss_check_socket(sk);
    }

    Socket* api_tcp_new_socket(Socket *temp_sk)
    {
        return tcp_new_socket(temp_sk);
    }

    void api_tcp_validate_socket(Socket *sk)
    {
        tcp_validate_socket(sk);
    }
    
    void api_tcp_reply(struct Socket *sk, uint8_t tcp_flags)
    {
        tcp_reply(sk, tcp_flags);
    }

    void api_tcp_release_socket(Socket *socket)
    {
        tcp_release_socket(socket);
    }

    void api_tcp_launch(Socket *socket)
    {
        tcp_launch(socket);
    }

    int api_process_tcp(Socket *socket, rte_mbuf *mbuf)
    {
        return socket->tcp.process(socket, mbuf);
    }

    void api_tcp_validate_csum(Socket* scoket)
    {
        tcp_validate_csum(scoket);
    }

    void api_tcp_validate_csum_opt(Socket* scoket)
    {
        tcp_validate_csum_opt(scoket);
    }

    void api_tcp_validate_csum_pkt(Socket *socket)
    {
        tcp_validate_csum_pkt(socket);
    }

    void api_tcp_validate_csum_data(Socket *socket)
    {
        tcp_validate_csum_data(socket);
    }

    void api_udp_set_payload(int page_size, int pattern_id)
    {
        udp_set_payload(page_size, pattern_id);
    }

    void api_udp_send(Socket *socket)
    {
        udp_send(socket);
    }

    char *api_udp_get_payload(int pattern_id)
    {
        return udp_get_payload(pattern_id);
    }

    void api_udp_validate_csum(Socket *socket)
    {
        udp_validate_csum(socket);
    }

    void api_enter_epoch()
    {
        dpdk_config_percore::enter_epoch();
    }

    int api_check_timer(int pattern)
    {
        return dpdk_config_percore::check_epoch_timer(pattern);
    }

    rte_mbuf* api_recv_packet()
    {
        return dpdk_config_percore::cfg_recv_packet();
    }

    Socket* api_parse_packet(rte_mbuf *mbuf)
    {
        return parse_packet(mbuf);
    }

    void api_send_packet(rte_mbuf *mbuf)
    {
        dpdk_config_percore::cfg_send_packet(mbuf);
    }

    void api_send_flush()
    {
        dpdk_config_percore::cfg_send_flush();
    }

    void api_time_update()
    {
        dpdk_config_percore::time_update();
    }

    Socket* api_flowtable_find_socket(Socket* ft_sk)
    {
        return TCP::socket_table->find_socket(ft_sk);
    }

    int api_flowtable_insert_socket(Socket* sk)
    {
        return TCP::socket_table->insert_socket(sk);
    }

    int api_flowtable_remove_socket(Socket* sk)
    {
        return TCP::socket_table->remove_socket(sk);
    }

    int api_flowtable_get_socket_count()
    {
        return TCP::socket_table->socket_table.size();
    }

    int api_http_ack_delay_flush(int pattern)
    {
        return http_ack_delay_flush(pattern);
    }

    void api_trigger_timers()
    {
        TIMERS.trigger();
    }

    void api_dpdk_run(int (*lcore_main)(void*), void* data)
    {
        dpdk_run(lcore_main, data);
    }

    void api_set_configs_and_init(dpdk_config_user& usrconfig, char** argv)
    {
        dpdk_config* dpdk_cfg = new dpdk_config(&usrconfig);
        dpdk_init(dpdk_cfg, argv[0]);
    }

    void api_set_pattern_num(int pattern_num)
    {
        g_pattern_num = pattern_num;
    }

    void api_config_protocols(int pattern, protocol_config *protocol_cfg)
    {
        config_protocols(pattern, protocol_cfg);
    }

    uint64_t& api_get_time_tsc()
    {
        return time_in_config();
    }

    int api_get_time_second()
    {
        return second_in_config();
    }
}