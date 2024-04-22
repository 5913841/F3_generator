#ifndef ETH_H
#define ETH_H

#include "common/define.h"
#include "protocols/base_protocol.h"
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <netinet/ether.h>
#include "dpdk/port.h"
#include "dpdk/dpdk_config.h"
#include "common/utils.h"
#include "common/packets.h"

static inline void eth_addr_swap(struct ether_header *eth)
{
    struct ether_addr addr;

    ether_addr_copy(addr, eth->ether_dhost);
    ether_addr_copy(eth->ether_dhost, eth->ether_shost);
    ether_addr_copy(eth->ether_shost, addr);
}

class ETHER;

extern thread_local ETHER *parser_eth;

class ETHER : public L2_Protocol
{
public:
    netif_port *port;
    ETHER() : L2_Protocol(), port(nullptr) {}
    ETHER(netif_port *port) : L2_Protocol(), port(port) {}
    ProtocolCode name() override { return ProtocolCode::PTC_ETH; }
    static inline ether_header *decode_hdr_pre(rte_mbuf *data)
    {
        return rte_pktmbuf_mtod_offset(data, struct ether_header *, -sizeof(struct ether_header));
    }
    static inline ether_header *decode_hdr(rte_mbuf *data)
    {
        return rte_pktmbuf_mtod_offset(data, struct ether_header *, 0);
    }
    size_t get_hdr_len(Socket *socket, rte_mbuf *data) override
    {
        return sizeof(struct ether_header);
    }
    size_t hash() override
    {
        return std::hash<uint8_t>()(PTC_ETH);
    }
    int construct(Socket *socket, rte_mbuf *data) override
    {
        ether_header *eth = decode_hdr_pre(data);

        if (socket->l3_protocol->name() == ProtocolCode::PTC_IPV4)
        {
            eth->ether_type = htons(RTE_ETHER_TYPE_IPV4);
        }
        else if (socket->l3_protocol->name() == ProtocolCode::PTC_IPV6)
        {
            eth->ether_type = htons(RTE_ETHER_TYPE_IPV6);
        }

        ether_header_init_addr(eth, port->local_mac, port->gateway_mac);

        data->l2_type = RTE_PTYPE_L2_ETHER;
        data->l2_len = sizeof(struct ether_header);
        rte_pktmbuf_prepend(data, sizeof(struct ether_header));

        return sizeof(struct ether_header);
    }
    int send_frame(Socket *socket, rte_mbuf *data) override
    {
        construct(socket, data);

        dpdk_config_percore::cfg_send_packet(data);
        return sizeof(struct ether_header);
    }

    static int parse_packet_ft(rte_mbuf *data, FiveTuples *ft, int offset)
    {
        ft->protocol_codes[SOCKET_L2] = ProtocolCode::PTC_ETH;
        return sizeof(struct ether_header);
    }

    static int parse_packet_sk(rte_mbuf *data, Socket *sk, int offset)
    {
        sk->l2_protocol = parser_eth;
        return sizeof(struct ether_header);
    }

    static void parser_init()
    {
        L2_Protocol::parser.add_parser(parse_packet_ft);
        L2_Protocol::parser.add_parser(parse_packet_sk);
    }
};

#endif