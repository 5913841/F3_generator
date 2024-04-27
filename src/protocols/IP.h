#ifndef IP_H
#define IP_H

#include "common/define.h"
#include "socket/socket.h"
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <netinet/ip.h>

extern __thread uint32_t ip_id;

class IPV4;

extern thread_local IPV4 *parser_ipv4;

class IPV4
{
public:
    static inline iphdr *decode_hdr_pre(rte_mbuf *data)
    {
        return rte_pktmbuf_mtod_offset(data, struct iphdr *, -sizeof(struct iphdr));
    }
    size_t get_hdr_len(Socket *socket, rte_mbuf *data)
    {
        return sizeof(struct iphdr);
    }
    inline int construct(Socket *socket, rte_mbuf *data)
    {
        iphdr *ip = decode_hdr_pre(data);
        ip->version = 4;
        ip->ihl = 5;
        ip->tos = 0;
        ip->tot_len = htons(data->data_len + sizeof(struct iphdr));
        ip->id = htons(ip_id++);
        ip->frag_off = 0;
        ip->ttl = DEFAULT_TTL;
        ip->frag_off = IP_FLAG_DF;
        ip->protocol = socket->protocol;
        ip->check = 0;
        ip->saddr = socket->src_addr;
        ip->daddr = socket->dst_addr;

        data->l3_len = sizeof(struct iphdr);
        rte_pktmbuf_prepend(data, sizeof(struct iphdr));
        return sizeof(struct iphdr);
    }
};

class IPV6
{
public:
    static inline ip6_hdr *decode_hdr_pre(rte_mbuf *data)
    {
        return rte_pktmbuf_mtod_offset(data, struct ip6_hdr *, -sizeof(struct ip6_hdr));
    }
};

#endif