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

// typedef std::function<int(rte_mbuf *, FiveTuples *, int)> parse_fivetuples_t;
// typedef std::function<int(rte_mbuf *, Socket *, int)> parse_socket_t;

typedef int (*parse_fivetuples_t)(rte_mbuf *, FiveTuples *, int);
typedef int (*parse_socket_t)(rte_mbuf *, Socket *, int);

struct PacketParser
{
    int parser_fivetuples_num;
    int parser_socket_num;
    parse_fivetuples_t parser_fivetuples[MAX_PROTOCOLS_PER_LAYER];
    parse_socket_t parser_socket[MAX_PROTOCOLS_PER_LAYER];
    int parse_packet(rte_mbuf *data, FiveTuples *ft, int offset);
    int parse_packet(rte_mbuf *data, Socket *socket, int offset);
    void add_parser(parse_fivetuples_t parse_func);
    void add_parser(parse_socket_t parse_func);
};

class Protocol
{
public:
    virtual ProtocolCode name(){ return PTC_NONE; }
    virtual int construct(Socket *socket, rte_mbuf *data) = 0;
    virtual size_t get_hdr_len(Socket *socket, rte_mbuf *data) = 0;
    virtual size_t hash() = 0;
};

class NamedProtocol : public Protocol
{
public:
    ProtocolCode code_;
    NamedProtocol(ProtocolCode code) : code_(code) {}
    virtual ProtocolCode name() override { return code_; }
    virtual int construct(Socket *socket, rte_mbuf *data) {return 0;};
    virtual size_t get_hdr_len(Socket *socket, rte_mbuf *data) {return 0;};
    virtual size_t hash() override {return std::hash<uint8_t>()(code_);};
};

class L2_Protocol : public Protocol
{
public:
    L2_Protocol() = default;
    static PacketParser parser;
    virtual ProtocolCode name() override { return PTC_NONE; }
    virtual int send_frame(Socket *socket, rte_mbuf *data) = 0;
};

class L3_Protocol : public Protocol
{
public:
    L3_Protocol() = default;
    static PacketParser parser;
    virtual ProtocolCode name() override { return PTC_NONE; }
};

class L4_Protocol : public Protocol
{
public:
    L4_Protocol() = default;
    static PacketParser parser;
    virtual ProtocolCode name() override { return PTC_NONE; }
    virtual int process(Socket *socket, rte_mbuf *data) = 0;
};

class L5_Protocol : public Protocol
{
public:
    L5_Protocol() = default;
    static PacketParser parser;
    virtual ProtocolCode name() override { return PTC_NONE; }
};

int parse_packet(rte_mbuf *data, FiveTuples *ft);
int parse_packet(rte_mbuf *data, Socket *sk);

#endif