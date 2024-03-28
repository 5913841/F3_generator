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

class Socket;

struct ipaddr_t
{
    union
    {
        struct in6_addr in6;
        struct
        {
            uint32_t pad[3];
            in_addr ip;
        };
    };
    ipaddr_t(in6_addr addr) : in6(addr) {}
    ipaddr_t(const std::string &ip_str)
    {
        std::string type = distinguish_address_type(ip_str);
        if (type == "IPv4")
        {
            inet_aton(ip_str.c_str(), &ip);
        }
        else if (type == "IPv6")
        {
            inet_pton(AF_INET6, ip_str.c_str(), &in6);
        }
        else
        {
            throw std::runtime_error("Invalid IP address: " + ip_str);
        }
    }
    ipaddr_t(uint32_t ip_)
    {
        ip.s_addr = htonl(ip_);
        memset(pad, 0, sizeof(pad));
    }
    ipaddr_t(const ipaddr_t &other) { in6 = other.in6; }
    ipaddr_t() { memset(&in6, 0, sizeof(in6)); }
    bool operator==(const ipaddr_t &other) const
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
    bool operator==(const FiveTuples &other) const;
    bool operator<(const FiveTuples &other) const;
    FiveTuples() { memset(this, 0, sizeof(FiveTuples)); }
    FiveTuples(const Socket &socket);

    static size_t hash(const FiveTuples &ft);
};

struct FiveTuplesHash
{
    size_t operator()(const FiveTuples &ft) const
    {
        return FiveTuples::hash(ft);
    }
};

struct Socket
{
public:
    union
    {
        Protocol *protocols[4];
        struct
        {
            L2_Protocol *l2_protocol;
            L3_Protocol *l3_protocol;
            L4_Protocol *l4_protocol;
            L5_Protocol *l5_protocol;
        };
    };
    ipaddr_t src_addr;
    ipaddr_t dst_addr;
    uint16_t src_port;
    uint16_t dst_port;
    Socket()
    {
        src_addr = ipaddr_t(0);
        dst_addr = ipaddr_t(0);
        src_port = 0;
        dst_port = 0;
        memset(protocols, 0, sizeof(protocols));
    };
    Socket(const Socket &other) : src_addr(other.src_addr), dst_addr(other.dst_addr), src_port(other.src_port), dst_port(other.dst_port)
    {
        memcpy(protocols, other.protocols, sizeof(protocols));
    }
    Socket(in6_addr src_addr, in6_addr dst_addr, uint16_t src_port, uint16_t dst_port) : src_addr(src_addr), dst_addr(dst_addr), src_port(src_port), dst_port(dst_port)
    {
        memset(protocols, 0, sizeof(protocols));
    }

    Socket(FiveTuples five_tuples) {
        src_addr = five_tuples.src_addr;
        dst_addr = five_tuples.dst_addr;
        src_port = five_tuples.src_port;
        dst_port = five_tuples.dst_port;
        protocols[0] = new NamedProtocol(five_tuples.protocol_codes[0]);
        protocols[1] = new NamedProtocol(five_tuples.protocol_codes[1]);
        protocols[2] = new NamedProtocol(five_tuples.protocol_codes[2]);
        protocols[3] = new NamedProtocol(five_tuples.protocol_codes[3]);
    }

    void set_protocol(int layer, Protocol *protocol)
    {
        protocols[layer] = protocol;
    }

    static size_t hash(const Socket &socket);

    static void delete_ft_created_socket(Socket* sk)
    {
        for(int i = 0; i < 4; i++)
        {
            if(sk->protocols[i] != nullptr)
            {
                delete sk->protocols[i];
                sk->protocols[i] = nullptr;
            }
        }
        delete sk;
    }
};

struct SocketPointerEqual
{
    bool operator()(const Socket *s1, const Socket *s2) const
    {
        return s1->src_addr == s2->src_addr &&
               s1->dst_addr == s2->dst_addr &&
               s1->src_port == s2->src_port &&
               s1->dst_port == s2->dst_port &&
               s1->l2_protocol->name() == s2->l2_protocol->name() &&
               s1->l3_protocol->name() == s2->l3_protocol->name() &&
               s1->l4_protocol->name() == s2->l4_protocol->name() &&
               s1->l5_protocol->name() == s2->l5_protocol->name();
    }
};

struct SocketPointerHash
{
    size_t operator()(const Socket *socket) const
    {
        return Socket::hash(*socket);
    }
};
#endif