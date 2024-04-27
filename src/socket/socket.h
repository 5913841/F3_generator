#ifndef SOCKET_H
#define SOCKET_H

#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <netinet/in.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdexcept>
#include "common/utils.h"
#include "protocols/TCP.h"
#include "protocols/HTTP.h"
#include "common/addrtype.h"

typedef __uint128_t uint128_t;

class Socket;

struct FiveTuples
{
    uint8_t protocol;
    uint16_t pad1;
    ip4addr_t src_addr;
    ip4addr_t dst_addr;
	port_t src_port;	/* source port */
	port_t dst_port;	/* destination port */
    uint8_t pad2;
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
    uint8_t protocol;
    uint16_t pad1;
    ip4addr_t src_addr;
    ip4addr_t dst_addr;
    port_t src_port;
    port_t dst_port;
    uint8_t pad2;
    TCP tcp;
    HTTP http;
    Socket()
    {
        src_addr = 0;
        dst_addr = 0;
        src_port = 0;
        dst_port = 0;
    };
    Socket(const Socket &other) : src_addr(other.src_addr), dst_addr(other.dst_addr), src_port(other.src_port), dst_port(other.dst_port), protocol(other.protocol)
    {
        tcp = other.tcp;
        http = other.http;
    }
    Socket(uint32_t src_addr, uint32_t dst_addr, uint16_t src_port, uint16_t dst_port) : src_addr(src_addr), dst_addr(dst_addr), src_port(src_port), dst_port(dst_port)
    {
    }

    Socket(FiveTuples five_tuples) {
        protocol = five_tuples.protocol;
        src_addr = five_tuples.src_addr;
        dst_addr = five_tuples.dst_addr;
        src_port = five_tuples.src_port;
        dst_port = five_tuples.dst_port;
    }


    static size_t hash(const Socket *socket);

    static size_t hash(const Socket &socket);
};

struct SocketPointerEqual
{
    bool operator()(const Socket *s1, const Socket *s2) const
    {
        return s1->src_addr == s2->src_addr &&
               s1->dst_addr == s2->dst_addr &&
               s1->src_port == s2->src_port &&
               s1->dst_port == s2->dst_port &&
               s1->protocol == s2->protocol;
        // return true;
    }
};

struct SocketPointerHash
{
    size_t operator()(const Socket *socket) const
    {
        return Socket::hash(socket);
    }
};
#endif