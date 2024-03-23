#ifndef SOCKET_H
#define SOCKET_H

#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <netinet/in.h>
#include "protocols/base_protocol.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdexcept>
#include "common/utils.h"

typedef __uint128_t uint128_t;

struct ipaddr_t {
    union {
        struct in6_addr in6;
        struct {
            uint32_t pad[3];
            in_addr ip;
        };
    };
    ipaddr_t(in6_addr addr) : in6(addr) {}
    ipaddr_t(const std::string& ip_str){
        std::string type = distinguish_address_type(ip_str);
        if (type == "IPv4") {
            inet_aton(ip_str.c_str(), &ip);
        } else if (type == "IPv6") {
            inet_pton(AF_INET6, ip_str.c_str(), &in6);
        } else {
            throw std::runtime_error("Invalid IP address: " + ip_str);
        }
    }
    ipaddr_t(uint32_t ip_) {ip.s_addr = htonl(ip_); memset(pad, 0, sizeof(pad));}
    ipaddr_t(const ipaddr_t& other) {in6 = other.in6;}
    ipaddr_t(){memset(&in6, 0, sizeof(in6));}
    bool operator == (const ipaddr_t& other) const
    {
        return IN6_ARE_ADDR_EQUAL(&in6, &other.in6);
    }
    operator uint32_t() const
    {
        return ntohl(ip.s_addr);
    }
};

struct FiveTuples
{
    ipaddr_t src_addr;
    ipaddr_t dst_addr;
    uint16_t src_port;
    uint16_t dst_port;
    ProtocolCode protocol_codes[4];
    bool operator == (const FiveTuples& other) const;
    bool operator < (const FiveTuples& other) const;
    FiveTuples(){memset(this, 0, sizeof(FiveTuples));}

    static size_t hash(const FiveTuples& ft);
};

struct FiveTuplesHash
{
    size_t operator()(const FiveTuples& ft) const
    {
        return FiveTuples::hash(ft);
    }
};

// template<typename l2_protocol_t = L2_Protocol, typename l3_protocol_t = L3_Protocol, typename l4_protocol_t = L4_Protocol, typename l5_protocol_t = L5_Protocol>
template <typename l2_protocol_t, typename l3_protocol_t, typename l4_protocol_t, typename l5_protocol_t>
struct Socket
{
public:
    ipaddr_t src_addr;
    ipaddr_t dst_addr;
    uint16_t src_port;
    uint16_t dst_port;
    l2_protocol_t l2_protocol;
    l3_protocol_t l3_protocol;
    l4_protocol_t l4_protocol;
    l5_protocol_t l5_protocol;
    Socket(){src_addr = ipaddr_t(0); dst_addr = ipaddr_t(0); src_port = 0; dst_port = 0; memset(protocols, 0, sizeof(protocols));};
    Socket(const Socket& other): src_addr(other.src_addr), dst_addr(other.dst_addr), src_port(other.src_port), dst_port(other.dst_port)
    {
        memcpy(protocols, other.protocols, sizeof(protocols));
    }
    Socket(in6_addr src_addr, in6_addr dst_addr, uint16_t src_port, uint16_t dst_port) : src_addr(src_addr), dst_addr(dst_addr), src_port(src_port), dst_port(dst_port)
    {
        memset(protocols, 0, sizeof(protocols));
    }

    // Socket(FiveTuples five_tuples_, ProtocolStack protocol_stack_) {
    //     four_tuples = five_tuples_.four_tuples;
    //     protocol_stack = protocol_stack_;
    // }

    operator FiveTuples() const
    {
        FiveTuples ft;
        memset(&ft, 0, sizeof(ft));
        ft.src_addr = src_addr;
        ft.dst_addr = dst_addr;
        ft.src_port = src_port;
        ft.dst_port = dst_port;
        ft.protocol_codes[0] = l2_protocol.code;
        ft.protocol_codes[1] = l3_protocol.code;
        ft.protocol_codes[2] = l4_protocol.code;
        ft.protocol_codes[3] = l5_protocol.code;
        return ft;
    }

    static size_t hash(const Socket& socket);
};

template<typename Socket>
struct SocketPointerEqual
{
    bool operator()(const Socket* s1, const Socket* s2) const
    {
        return s1->src_addr == s2->src_addr &&
                s1->dst_addr == s2->dst_addr &&
                s1->src_port == s2->src_port &&
                s1->dst_port == s2->dst_port &&
                s1->l2_protocol->name == s2->l2_protocol->name &&
                s1->l3_protocol->name == s2->l3_protocol->name &&
                s1->l4_protocol->name == s2->l4_protocol->name &&
                s1->l5_protocol->name == s2->l5_protocol->name;
    }
};

template<typename Socket>
struct SocketPointerHash 
{
    size_t operator()(const Socket* socket) const
    {
        return Socket::hash(*socket);
    }
};
#endif