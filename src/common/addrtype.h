#ifndef ADDRTYPE_H
#define ADDRTYPE_H

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

struct ip4addr_t;

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
        pad[0] = 0;
        pad[1] = 0;
        pad[2] = 0;
        ip.s_addr = htonl(ip_);
    }
    ipaddr_t(const ipaddr_t &other) { in6 = other.in6; }
    ipaddr_t(const ip4addr_t &other);
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

struct ip4addr_t 
{
    in_addr ip;
    ip4addr_t(const std::string &ip_str)
    {
        std::string type = distinguish_address_type(ip_str);
        if (type == "IPv4")
        {
            inet_aton(ip_str.c_str(), &ip);
        }
        else
        {
            throw std::runtime_error("Invalid IP address: " + ip_str);
        }
    }
    ip4addr_t(uint32_t ip_)
    {
        ip.s_addr = htonl(ip_);
    }
    ip4addr_t() { memset(&ip, 0, sizeof(ip)); }
    ip4addr_t(const ipaddr_t &other) { ip = other.ip; }
    bool operator==(const ipaddr_t &other) const
    {
        return ip.s_addr == other.ip.s_addr;
    }
    operator uint32_t() const
    {
        return ntohl(ip.s_addr);
    }
};

struct port_t
{
    uint16_t port;
    port_t()
    {
        port = 0;
    }
    port_t(uint16_t port_)
    {
        port = htons(port_);
    }
    operator uint16_t() const
    {
        return ntohs(port);
    }
};

#endif
