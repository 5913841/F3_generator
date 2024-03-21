#ifndef BASE_PROTOCOL_H
#define BASE_PROTOCOL_H

#include "rte_mbuf.h"
#include <functional>
#include "common/define.h"

class Socket;
class FiveTuples;

enum ProtocolCode
{
    PTC_NONE = 0,
    PTC_IPV4 = 1,
    PTC_IPV6,
    PTC_TCP,
    PTC_UDP,
    PTC_ICMP,
    PTC_ICMPv6,
    PTC_ARP,
    PTC_HTTP,
    PTC_HTTPS,
    PTC_HTTP2,
    PTC_ETH,
};

typedef std::function<int(rte_mbuf*, FiveTuples*, int)> parse_fivetuples_t;
typedef std::function<int(rte_mbuf*, Socket*, int)> parse_socket_t;

struct PacketParser {
    int parser_fivetuples_num;
    int parser_socket_num;
    parse_fivetuples_t parser_fivetuples[MAX_PROTOCOLS_PER_LAYER];
    parse_socket_t parser_socket[MAX_PROTOCOLS_PER_LAYER];
    int parse_packet(rte_mbuf* data, FiveTuples* ft, int offset);
    int parse_packet(rte_mbuf* data, Socket* socket, int offset);
    void add_parser(parse_fivetuples_t parse_func);
    void add_parser(parse_socket_t parse_func);
};


class Protocol {
public:
    ProtocolCode name; 
    Protocol() : name(PTC_NONE) {};
    Protocol(ProtocolCode name) {this->name = name;}
    virtual int construct(Socket* socket, rte_mbuf* data) = 0;
    virtual size_t get_hdr_len(Socket* socket, rte_mbuf* data) = 0;
};

class L2_Protocol : public Protocol {
public:
    L2_Protocol() = default;
    L2_Protocol(ProtocolCode name) {this->name = name;}
    static PacketParser parser;
    virtual int send_frame(Socket* socket, rte_mbuf* data) = 0;
};


class L3_Protocol : public Protocol {
public:
    L3_Protocol() = default;
    L3_Protocol(ProtocolCode name) {this->name = name;}
    static PacketParser parser;
};

class L4_Protocol : public Protocol {
public:
    L4_Protocol() = default;
    L4_Protocol(ProtocolCode name) {this->name = name;}
    static PacketParser parser;
    virtual int process(Socket* socket, rte_mbuf* data) = 0;
};

class L5_Protocol : public Protocol {
public:
    L5_Protocol() = default;
    L5_Protocol(ProtocolCode name) {this->name = name;}
    static PacketParser parser;
};

int parse_packet(rte_mbuf* data, FiveTuples* ft);
int parse_packet(rte_mbuf* data, Socket* sk);

#endif