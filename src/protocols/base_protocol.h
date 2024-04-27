#ifndef BASE_PROTOCOL_H
#define BASE_PROTOCOL_H

#include "rte_mbuf.h"
#include <functional>
#include "common/define.h"
#include <netinet/ether.h>
#include <netinet/ip.h>
#include "socket/socket.h"


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


inline int parse_packet(rte_mbuf *data, FiveTuples *ft)
{
    // __m128i tmpdata0 = _mm_loadu_si128(rte_pktmbuf_mtod_offset(data, __m128i *, sizeof(ether_header) + offsetof(iphdr, ttl)));
    // _mm_storeu_si128((__m128i *) ft, tmpdata0);
    // uint32_t src_addr = ft->src_addr;
    // ft->src_addr = ft->dst_addr;
    // ft->dst_addr = src_addr;
    // uint16_t src_port = ft->src_port;
    // ft->src_port = ft->dst_port;
    // ft->dst_port = src_port;
    ether_header *eth_hdr = rte_pktmbuf_mtod(data, struct ether_header *);
    iphdr *ip_hdr = rte_pktmbuf_mtod_offset(data, struct iphdr *, sizeof(ether_header));
    tcphdr *tcp_hdr = rte_pktmbuf_mtod_offset(data, struct tcphdr *, sizeof(ether_header)+sizeof(iphdr));
    ft->src_addr = ntohl(ip_hdr->daddr);
    ft->dst_addr = ntohl(ip_hdr->saddr);
    ft->src_port = ntohs(tcp_hdr->dest);
    ft->dst_port = ntohs(tcp_hdr->source);
    ft->protocol = ip_hdr->protocol;
    return 0;
}
inline int parse_packet(rte_mbuf *data, Socket *sk)
{
    // __m128i tmpdata0 = _mm_loadu_si128(rte_pktmbuf_mtod_offset(data, __m128i *, sizeof(ether_header) + offsetof(iphdr, ttl)));
    // _mm_storeu_si128((__m128i *) sk, tmpdata0);
    // uint32_t src_addr = sk->src_addr;
    // sk->src_addr = sk->dst_addr;
    // sk->dst_addr = src_addr;
    // uint16_t src_port = sk->src_port;
    // sk->src_port = sk->dst_port;
    // sk->dst_port = src_port;
    ether_header *eth_hdr = rte_pktmbuf_mtod(data, struct ether_header *);
    iphdr *ip_hdr = rte_pktmbuf_mtod_offset(data, struct iphdr *, sizeof(ether_header));
    tcphdr *tcp_hdr = rte_pktmbuf_mtod_offset(data, struct tcphdr *, sizeof(ether_header)+sizeof(iphdr));
    sk->src_addr = ntohl(ip_hdr->daddr);
    sk->dst_addr = ntohl(ip_hdr->saddr);
    sk->src_port = ntohs(tcp_hdr->dest);
    sk->dst_port = ntohs(tcp_hdr->source);
    sk->protocol = ip_hdr->protocol;
    return 0;
}

#endif