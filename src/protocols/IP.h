#ifndef IP_H
#define IP_H

#include "common/define.h"
#include "protocols/base_protocol.h"
#include "socket/socket.h"
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <netinet/ip.h>

extern __thread uint32_t ip_id;

class IPV4;

extern IPV4 *parser_ipv4;

class IPV4 : public L3_Protocol
{
public:
    IPV4() : L3_Protocol() {}
    ProtocolCode name() override { return ProtocolCode::PTC_IPV4; }
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
        if (socket->l4_protocol->name() == ProtocolCode::PTC_TCP)
        {
            ip->protocol = IPPROTO_TCP;
        }
        else if (socket->l4_protocol->name() == ProtocolCode::PTC_UDP)
        {
            ip->protocol = IPPROTO_UDP;
        }
        else if (socket->l4_protocol->name() == ProtocolCode::PTC_ICMP)
        {
            ip->protocol = IPPROTO_ICMP;
        }
        ip->check = 0;
        ip->saddr = socket->src_addr;
        ip->daddr = socket->dst_addr;

        data->l3_len = sizeof(struct iphdr);
        rte_pktmbuf_prepend(data, sizeof(struct iphdr));
        return sizeof(struct iphdr);
    }

    static int parse_packet_ft(rte_mbuf *data, FiveTuples *ft, int offset)
    {
        ft->protocol_codes[SOCKET_L3] = ProtocolCode::PTC_IPV4;
        iphdr *ip = rte_pktmbuf_mtod_offset(data, struct iphdr *, offset);
        // exchange src and dst
        ft->src_addr = ntohl(ip->daddr);
        ft->dst_addr = ntohl(ip->saddr);
        return sizeof(struct iphdr);
    }

    static int parse_packet_sk(rte_mbuf *data, Socket *sk, int offset)
    {
        sk->l3_protocol = parser_ipv4;
        iphdr *ip = rte_pktmbuf_mtod_offset(data, struct iphdr *, offset);
        // exchange src and dst
        sk->src_addr.ip.s_addr = ip->daddr;
        sk->dst_addr.ip.s_addr = ip->saddr;
        return sizeof(struct iphdr);
    }

    static void parser_init()
    {
        L3_Protocol::parser.add_parser(parse_packet_ft);
        L3_Protocol::parser.add_parser(parse_packet_sk);
    }
};

class IPV6 : public Protocol
{
public:
    IPV6() : Protocol() {}
    ProtocolCode name() override { return ProtocolCode::PTC_IPV6; }
    static inline ip6_hdr *decode_hdr_pre(rte_mbuf *data)
    {
        return rte_pktmbuf_mtod_offset(data, struct ip6_hdr *, -sizeof(struct ip6_hdr));
    }
};

#endif