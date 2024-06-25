#include "dpdk/divert/rss.h"
#include "protocols/conf_protocol.h"
#include "protocols/global_states.h"

namespace api{

    typedef phmap::parallel_flat_hash_set<Socket *, SocketPointerHash, SocketPointerEqual> socket_set_t;



    socket_set_t& api_get_flowtable() // return the flowtable
    {
        return TCP::socket_table->socket_table;
    }

    uint8_t api_get_syn_pattern_by_res(rte_mbuf *mbuf) // return the syn pattern by res field in tcp header, directly pick the res field from the packet as the pattern.
    {
        return rte_pktmbuf_mtod_offset(mbuf, tcphdr*, sizeof(ethhdr) + sizeof(iphdr))->res1;
    }

    const uint8_t& api_get_packet_l4_protocol(rte_mbuf *m) // return the l4 protocol of the packet, either IPPROTO_TCP or IPPROTO_UDP.
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

    bool api_socket_thscore(Socket *sk) // return true if the socket is in the RSS table.
    {
        return rss_check_socket(sk);
    }

    void api_free_mbuf(rte_mbuf* mbuf) // free the mbuf.
    {
        mbuf_free2(mbuf);
    }

    Socket* api_tcp_new_socket(Socket *temp_sk) // create a new socket same as the temp_sk and return it.
    {
        return tcp_new_socket(temp_sk);
    }

    void api_tcp_validate_socket(Socket *sk) // validate the socket. include the checksum, state, timer.
    {
        tcp_validate_socket(sk);
    }
    
    void api_tcp_reply(struct Socket *sk, uint8_t tcp_flags) // send a tcp reply packet to the opposite side of the socket according to the socket state and tcp_flags.
    {
        tcp_reply(sk, tcp_flags);
    }

    void api_tcp_release_socket(Socket *socket) // release(delete) the socket.
    {
        tcp_release_socket(socket);
    }

    void api_tcp_launch(Socket *socket) // launch the socket, include valadate the socket and send a tcp syn packet.
    {
        tcp_launch(socket);
    }

    int api_process_tcp(Socket *socket, rte_mbuf *mbuf) // process the mbuf packet according to the socket state by running a simplified tcp state machine.
    {
        return socket->tcp.process(socket, mbuf);
    }

    void api_tcp_validate_csum(Socket* scoket) // validate the checksum of the socket.
    {
        tcp_validate_csum(scoket);
    }

    void api_tcp_validate_csum_opt(Socket* scoket) // just validate the checksum of the option packets of the socket.
    {
        tcp_validate_csum_opt(scoket);
    }

    void api_tcp_validate_csum_pkt(Socket *socket) // just validate the checksum of the packet of the socket.
    {
        tcp_validate_csum_pkt(socket);
    }

    void api_tcp_validate_csum_data(Socket *socket) // just validate the checksum of the data packets of the socket.
    {
        tcp_validate_csum_data(socket);
    }

    void api_udp_set_payload(int page_size, int pattern_id) // set the payload of the packet of pattern_id th udp pattern, the payload size is page_size.
    {
        udp_set_payload(page_size, pattern_id);
    }

    void api_udp_send(Socket *socket) // send a udp packet to the opposite side of the socket.
    {
        udp_send(socket);
    }

    char *api_udp_get_payload(int pattern_id) // return the payload of the packet of pattern_id th udp pattern.
    {
        return udp_get_payload(pattern_id);
    }

    void api_udp_validate_csum(Socket *socket) // validate the checksum of the udp packet of the socket.
    {
        udp_validate_csum(socket);
    }

    void api_enter_epoch() // things to do when entering epoch, include updating the time, print the speed.
    {
        dpdk_config_percore::enter_epoch();
    }

    int api_check_timer(int pattern) // check how many sockets should be launch in the pattern, return the number of sockets to be launched.
    {
        return dpdk_config_percore::check_epoch_timer(pattern);
    }

    rte_mbuf* api_recv_packet() // receive a packet from the dpdk NIC.
    {
        return dpdk_config_percore::cfg_recv_packet();
    }

    Socket* api_parse_packet(rte_mbuf *mbuf) // parse the packet and return the five-tuples of the packet, there are just five-tuples valid in the returned packet.
    {
        return parse_packet(mbuf);
    }

    void api_set_ft_from_parse(Socket* sk, Socket* parse_sk) // set the five-tuples of the socket from the parsed socket.
    {
        memcpy(sk, parse_sk, sizeof(FiveTuples));
    }

    void api_send_packet(rte_mbuf *mbuf) // send a packet to the dpdk NIC.
    {
        dpdk_config_percore::cfg_send_packet(mbuf);
    }

    void api_send_flush() // flush the packets to send in the dpdk NIC.
    {
        dpdk_config_percore::cfg_send_flush();
    }

    void api_time_update() // update the time of the global timer, should be called when you need accurate time (be called in api_enter_epoch() so at least update a time per epoch).
    {
        dpdk_config_percore::time_update();
    }

    Socket* api_flowtable_find_socket(Socket* ft_sk) // find the socket in the flowtable by the five-tuples of the socket.
    {
        return TCP::socket_table->find_socket(ft_sk);
    }

    int api_flowtable_insert_socket(Socket* sk) // insert the socket into the flowtable.
    {
        return TCP::socket_table->insert_socket(sk);
    }

    int api_flowtable_remove_socket(Socket* sk) // remove socket by the five-tuples of the socket provided from the flowtable.
    {
        return TCP::socket_table->remove_socket(sk);
    }

    int api_flowtable_get_socket_count() // return the number of sockets in the flowtable.
    {
        return TCP::socket_table->socket_table.size();
    }

    Socket* api_range_find_socket(Socket* sk, int pattern) // find the socket in the range table by the five-tuples of the socket.
    {
        return g_vars[pattern].tcp_vars.socket_range_table->find_socket(sk);
    }

    int api_range_insert_socket(Socket*& sk, int pattern) // change the arg pointer to the socket pointer in the table whose five-tuples is the same as the socket provided and set valid the socket in table. just simulate the insert operation.
    {
        return g_vars[pattern].tcp_vars.socket_range_table->insert_socket(sk);
    }

    int api_range_remove_socket(Socket* sk, int pattern) // remove(set invalid) the socket from the range table by the five-tuples of the socket provided.
    {
        return g_vars[pattern].tcp_vars.socket_range_table->remove_socket(sk);
    }

    Socket* api_ptr_range_find_socket(Socket* sk, int pattern) // find the socket in the pointer range table by the five-tuples of the socket.
    {
        return g_vars[pattern].tcp_vars.socket_pointer_range_table->find_socket(sk);
    }

    int api_ptr_range_insert_socket(Socket* sk, int pattern) // insert the socket into the pointer range table.
    {
        return g_vars[pattern].tcp_vars.socket_pointer_range_table->insert_socket(sk);
    }

    int api_ptr_range_remove_socket(Socket* sk, int pattern) // remove the socket from the pointer range table by the five-tuples of the socket provided.
    {
        return g_vars[pattern].tcp_vars.socket_pointer_range_table->remove_socket(sk);
    }

    Socket* api_tcp_find_socket(Socket* sk, int pattern) // find the socket by the five-tuples of the socket, according to the type of the pattern.
    {
        return tcp_find_socket(sk, pattern);
    }

    int api_tcp_insert_socket(Socket*& sk, int pattern) // insert the socket into the table, according to the type of the pattern.
    {
        return tcp_insert_socket(sk, pattern);
    }

    int api_tcp_remove_socket(Socket* sk, int pattern) // remove the socket from the table, according to the type of the pattern.
    {
        return tcp_remove_socket(sk, pattern);
    }

    int api_http_ack_delay_flush(int pattern) // flush the http ack delay queue of the pattern.
    {
        return http_ack_delay_flush(pattern);
    }

    void api_trigger_timers() // trigger all the tcp timers.
    {
        TIMERS.trigger();
    }

    void api_dpdk_run(int (*lcore_main)(void*), void* data) // run the dpdk main function.
    {
        dpdk_run(lcore_main, data);
    }

    void api_set_configs_and_init(dpdk_config_user& usrconfig, char** argv) // set the dpdk configurations and initialize the dpdk according to the user configuration.
    {
        dpdk_config* dpdk_cfg = new dpdk_config(&usrconfig);
        dpdk_init(dpdk_cfg, argv[0]);
    }

    void api_set_pattern_num(int pattern_num) // set the total number of patterns.
    {
        g_pattern_num = pattern_num;
    }

    void api_config_protocols(int pattern, protocol_config *protocol_cfg) // configure and initialize the pattern th protocol of the pattern, according to the protocol_cfg provided.
    {
        config_protocols(pattern, protocol_cfg);
    }

    uint64_t& api_get_time_tsc() // return the current time in tsc.
    {
        return time_in_config();
    }

    int api_get_time_second() // return the current time in second.
    {
        return second_in_config();
    }
}