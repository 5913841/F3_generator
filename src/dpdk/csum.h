#ifndef CUMS_H
#define CUMS_H

#include "dpdk/dpdk.h"
#include "dpdk/port.h"
#include "dpdk/mbuf.h"
#include "common/define.h"
#include <rte_mbuf.h>
#include <rte_eal.h>
#include <rte_ether.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/ether.h>

static inline uint16_t csum_update_u32(uint16_t ocsum, uint32_t oval, uint32_t nval)
{
    uint32_t csum = 0;

    csum = (~(oval >> 16) & 0xFFFF) + ((~(oval & 0xFFFF)) & 0xFFFF) + (~(nval >> 16) & 0xFFFF) + ((~(nval & 0xFFFF)) & 0xFFFF) + ocsum;
    csum = (csum & 0xFFFF) + (csum >> 16);
    csum = (csum & 0xFFFF) + (csum >> 16);

    return csum;
}

static inline uint16_t csum_update_u128(uint16_t ocsum, uint32_t *oval, uint32_t *nval)
{
    int i = 0;
    uint32_t csum = 0;

    csum = ocsum;
    for (i = 0; i < 4; i++)
    {
        csum = csum_update_u32(csum, oval[i], nval[i]);
    }

    return csum;
}

static inline uint16_t csum_update_u16(uint16_t ocsum, uint16_t oval, uint16_t nval)
{
    uint32_t csum = 0;

    csum = (~ocsum & 0xFFFF) + (~oval & 0xFFFF) + nval;
    csum = (csum >> 16) + (csum & 0xFFFF);
    csum += (csum >> 16);

    return ~csum;
}

static inline void csum_ipv4(struct rte_mbuf *m, int offload)
{
    struct iphdr *iph = NULL;

    iph = rte_pktmbuf_mtod_offset(m, iphdr *, sizeof(ether_header));
    iph->check = 0;
    if (offload != 0)
    {
        m->ol_flags |= RTE_MBUF_F_TX_IP_CKSUM;
        m->l2_len = sizeof(ether_header);
        m->l3_len = sizeof(iphdr);
    }
    else
    {
        iph->check = rte_ipv4_cksum((const struct rte_ipv4_hdr *)iph);
    }
}

static inline void csum_ip_offload(struct rte_mbuf *m)
{
    csum_ipv4(m, g_dev_tx_offload_ipv4_cksum);
}

static inline void csum_ip_compute(struct rte_mbuf *m)
{
    csum_ipv4(m, 0);
}

static inline void csum_offload_ip_tcpudp(struct rte_mbuf *m, uint64_t ol_flags)
{
    struct ip6_hdr *ip6h = NULL;
    struct iphdr *iph = NULL;
    struct tcphdr *th = NULL;

    iph = rte_pktmbuf_mtod_offset(m, iphdr *, sizeof(ether_header));
    th = rte_pktmbuf_mtod_offset(m, tcphdr *, sizeof(ether_header) + sizeof(iphdr));
    m->l2_len = sizeof(ether_header);

    if (iph->version == 4)
    {
        m->l3_len = sizeof(iphdr);
        iph->ttl = DEFAULT_TTL;
        iph->check = 0;

        // if (unlikely(g_dev_tx_offload_tcpudp_cksum == 0)) {
        th->th_sum = 0;
        th->th_sum = rte_ipv4_udptcp_cksum((const struct rte_ipv4_hdr *)iph, th);
        // } else {
        //     m->ol_flags = ol_flags | RTE_MBUF_F_TX_IPV4;
        // }

        // if (g_dev_tx_offload_ipv4_cksum) {
        //     m->ol_flags |= RTE_MBUF_F_TX_IP_CKSUM | RTE_MBUF_F_TX_IPV4;
        // } else {
        iph->check = rte_ipv4_cksum((const struct rte_ipv4_hdr *)iph);
        // }
    }
    else
    {
        ip6h = (struct ip6_hdr *)iph;
        ip6h->ip6_hops = DEFAULT_TTL;
        // if (unlikely(g_dev_tx_offload_tcpudp_cksum == 0)) {
        th->th_sum = 0;
        th->th_sum = rte_ipv6_udptcp_cksum((const struct rte_ipv6_hdr *)iph, (const void *)th);
        // } else {
        //     m->l3_len = sizeof(struct ip6_hdr);
        //     m->ol_flags = ol_flags | RTE_MBUF_F_TX_IPV6;
        // }
    }
}

struct Socket;
int csum_check(struct rte_mbuf *m);

#endif // __CUMS_H