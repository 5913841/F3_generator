#ifndef ETH_H
#define ETH_H

#include "common/define.h"
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <netinet/ether.h>
#include "dpdk/port.h"
#include "dpdk/dpdk_config.h"
#include "common/utils.h"
#include "common/packets.h"
#include "socket/socket.h"

static inline void eth_addr_swap(struct ether_header *eth)
{
    struct ether_addr addr;

    ether_addr_copy(addr, eth->ether_dhost);
    ether_addr_copy(eth->ether_dhost, eth->ether_shost);
    ether_addr_copy(eth->ether_shost, addr);
}

class ETHER;

extern thread_local ETHER *parser_eth;

class ETHER
{
public:
    netif_port *port;
    ETHER(): port(nullptr) {}
    ETHER(netif_port *port): port(port) {}
    static inline ether_header *decode_hdr_pre(rte_mbuf *data)
    {
        return rte_pktmbuf_mtod_offset(data, struct ether_header *, -sizeof(struct ether_header));
    }
    static inline ether_header *decode_hdr(rte_mbuf *data)
    {
        return rte_pktmbuf_mtod_offset(data, struct ether_header *, 0);
    }
    int construct(Socket *socket, rte_mbuf *data)
    {
        ether_header *eth = decode_hdr_pre(data);

        eth->ether_type = htons(RTE_ETHER_TYPE_IPV4);
        ether_header_init_addr(eth, port->local_mac, port->gateway_mac);

        data->l2_type = RTE_PTYPE_L2_ETHER;
        data->l2_len = sizeof(struct ether_header);
        rte_pktmbuf_prepend(data, sizeof(struct ether_header));

        return sizeof(struct ether_header);
    }
};

#endif