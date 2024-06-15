#ifndef CONF_PROTOCOL_H
#define CONF_PROTOCOL_H

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

extern thread_local struct ETHER *ether;
extern thread_local struct IPV4 ip[MAX_PATTERNS];
extern thread_local struct Socket template_socket[MAX_PATTERNS];

extern thread_local mbuf_cache template_tcp_data[MAX_PATTERNS];
extern thread_local mbuf_cache template_tcp_opt[MAX_PATTERNS];
extern thread_local mbuf_cache template_tcp_pkt[MAX_PATTERNS];
extern thread_local char const* data[MAX_PATTERNS];

struct protocol_config {
    std::string protocol = "TCP";
    std::string mode = "client";
    std::string gen_mode = "default";
    bool preset = false;
    bool use_http = false;
    bool use_keepalive = false;
    std::string keepalive_interval = "1ms";
    std::string keepalive_timeout = "0s";
    std::string keepalive_request_maxnum = "0";
    bool just_send_first_packet = false;
    std::string launch_batch = "4";
    bool payload_random = false;
    std::string payload_size = "80";
    std::string send_window = "0";
    std::string mss = std::to_string(MSS_IPV4);
    std::string cps = "1m";
    std::string tos = "0";
    std::string cc = "0";
    int slow_start = 0;
    std::string template_ip_src = "192.168.1.1";
    std::string template_ip_dst = "192.168.1.2";
    std::string template_port_src = "80";
    std::string template_port_dst = "80";
};


char *config_str_find_nondigit(char *s, bool float_enable);

int config_parse_number(const char *str, bool float_enable, bool rate_enable);

uint64_t config_parse_time(const char *str);

void config_protocols(int pattern, protocol_config *protocol_cfg);

#endif /* PROTOCOL_CONF_H */