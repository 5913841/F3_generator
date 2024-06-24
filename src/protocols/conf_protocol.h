#ifndef CONF_PROTOCOL_H
#define CONF_PROTOCOL_H

#include "socket/socket.h"
#include "socket/ftrange.h"
#include "dpdk/dpdk.h"
#include "dpdk/dpdk_config.h"
#include "dpdk/dpdk_config_user.h"
#include "protocols/ETH.h"
#include "protocols/IP.h"
#include "protocols/TCP.h"
#include "protocols/HTTP.h"
#include "socket/socket_table/socket_table.h"
#include "socket/socket_tree/socket_tree.h"
#include "socket/socket_range/socket_prange.h"
#include "socket/socket_range/socket_range.h"
#include "protocols/base_protocol.h"
#include <rte_lcore.h>

extern thread_local struct ETHER *ether;
extern thread_local struct IPV4 ip[MAX_PATTERNS];
extern thread_local struct Socket template_socket[MAX_PATTERNS];

extern thread_local mbuf_cache template_tcp_data[MAX_PATTERNS];
extern thread_local mbuf_cache template_tcp_opt[MAX_PATTERNS];
extern thread_local mbuf_cache template_tcp_pkt[MAX_PATTERNS];
extern thread_local char const* data[MAX_PATTERNS];

struct protocol_config {
    std::string protocol = "TCP"; // The l4 protocol we choose to use, default TCP.
    std::string mode = "client"; // Choose client or server to begin the task.
    bool preset = false; // Whether to preset five-tuples and its pattern by add five-tuples. If not, user should set random function in client.
    bool use_flowtable = true; // Whether to use hash flowtable to store the five-tuples.
    bool use_http = false; // Whether to run the simplified http protocol on TCP.
    bool use_keepalive = false; // Whether to turn on TCP keepalive.
    std::string keepalive_interval = "1ms"; // The interval of client sending keepalive packet.
    std::string keepalive_timeout = "0s"; // If maxnum not set, use time out time to ascertain it.
    std::string keepalive_request_maxnum = "0"; // The max number for client to send keepalive packet.
    bool just_send_first_packet = false; // 
    std::string launch_batch = "4"; // Flows launched together in one epoch.
    bool payload_random = false; // Randomize the payload of the packet.
    std::string payload_size = "80"; // Set the size of the payload of the packet sent.
    std::string send_window = "0"; // The send window of the TCP protocol.
    std::string mss = std::to_string(MSS_IPV4); // The max segment size of the protocol.
    std::string cps = "1m"; // To control the rate, set connections launched per second.
    std::string tos = "0"; // To control the rate, set maximum connections to launch.
    std::string cc = "0"; // Set the tos field of the packet.
    int slow_start = 0; // Seconds from task begin to the speed increase to the max.
    std::string template_ip_src = "192.168.1.1"; // The source address of template socket under this pattern.
    std::string template_ip_dst = "192.168.1.2"; // The destination address of template socket under this pattern.
    std::string template_port_src = "80"; // The source port of template socket under this pattern.
    std::string template_port_dst = "80"; // The destination port of template socket under this pattern.
    FTRange ft_range; // When not use the flow table, set ft range to control the five-tuples.
};


char *config_str_find_nondigit(char *s, bool float_enable);

int config_parse_number(const char *str, bool float_enable, bool rate_enable);

uint64_t config_parse_time(const char *str);

void config_protocols(int pattern, protocol_config *protocol_cfg);

#endif /* PROTOCOL_CONF_H */