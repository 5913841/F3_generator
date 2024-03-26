#include "dpdk/dpdk_config.h"
#include "dpdk/port.h"
#include "common/define.h"
#include "dpdk/mbuf.h"
#include "dpdk/bond/bond.h"

#include <rte_ethdev.h>
#include <rte_version.h>

uint8_t g_dev_tx_offload_ipv4_cksum;
uint8_t g_dev_tx_offload_tcpudp_cksum;

static struct rte_eth_conf g_port_conf = {
    .rxmode = {
        .mq_mode = RTE_ETH_MQ_RX_NONE,
#if RTE_VERSION < RTE_VERSION_NUM(21, 11, 0, 0)
        .max_rx_pkt_len = ETHER_MAX_LEN,
#endif
#if RTE_VERSION < RTE_VERSION_NUM(22, 11, 0, 0)
        .split_hdr_size = 0,
#endif
#if RTE_VERSION < RTE_VERSION_NUM(18, 11, 0, 0)
        .hw_ip_checksum = 1,
        .hw_vlan_strip = 1,
        .hw_strip_crc = 1,
#endif
    },
    .txmode = {
        .mq_mode = RTE_ETH_MQ_TX_NONE,
    },
    .rx_adv_conf = {
        .rss_conf = {
            .rss_key = NULL,
            .rss_hf = 0,
        },
    },
};

static int port_init_tx(int port_id, int txq, int nb_txd)
{
    unsigned int socket = rte_eth_dev_socket_id(port_id);
    return rte_eth_tx_queue_setup(port_id, txq, nb_txd, socket, NULL);
}

static int port_init_rx(struct netif_port *port, int rxq, int nb_rxd)
{
    int port_id = port->id;
    struct rte_mempool *mp = port_get_mbuf_pool(port, rxq);
    unsigned int socket = rte_eth_dev_socket_id(port_id);

    return rte_eth_rx_queue_setup(port_id, rxq, nb_rxd, socket, NULL, mp);
}

static int port_init_mbuf_pool(dpdk_config *cfg, netif_port *port)
{
    int i = 0;
    struct rte_mempool *mbuf_pool = NULL;

    for (i = 0; i < port->queue_num; i++)
    {
        mbuf_pool = mbuf_pool_create(cfg, "mp", port->id, i);
        if (mbuf_pool == NULL)
        {
            return -1;
        }
        port->mbuf_pool[i] = mbuf_pool;
    }

    return 0;
}

static int port_config_vlan(struct rte_eth_conf *conf, struct rte_eth_dev_info *dev_info)
{
    if (dev_info->tx_offload_capa & RTE_ETH_TX_OFFLOAD_VLAN_INSERT)
    {
        conf->txmode.offloads |= RTE_ETH_TX_OFFLOAD_VLAN_INSERT;
    }
    else
    {
        printf("Error: port cannot insert vlan\n");
        return -1;
    }

    if (dev_info->rx_offload_capa & RTE_ETH_RX_OFFLOAD_VLAN_STRIP)
    {
        conf->rxmode.offloads |= RTE_ETH_RX_OFFLOAD_VLAN_STRIP;
    }
    else
    {
        printf("Error: port cannot strip vlan\n");
        return -1;
    }

    return 0;
}

int port_config(dpdk_config *cfg, struct netif_port *port)
{
    int i = 0;
    int nb_txd = NB_TXD;
    int nb_rxd = NB_RXD;
    int queue_num = 0;
    uint16_t port_id = 0;
    struct rte_eth_dev_info dev_info;

    port_id = port->id;
    queue_num = port->queue_num;
    memset(&dev_info, 0, sizeof(dev_info));
    rte_eth_dev_info_get(port_id, &dev_info);

    if (nb_rxd > dev_info.rx_desc_lim.nb_max)
    {
        nb_rxd = dev_info.rx_desc_lim.nb_max;
    }

    if (nb_txd > dev_info.tx_desc_lim.nb_max)
    {
        nb_txd = dev_info.tx_desc_lim.nb_max;
    }

    if (cfg->vlan_id)
    {
        if (port_config_vlan(&g_port_conf, &dev_info) < 0)
        {
            return -1;
        }
    }

    // if (cfg->rss) {
    //     if (rss_config_port(&g_port_conf, &dev_info) < 0) {
    //         printf("Error: rss config port error\n");
    //         return -1;
    //     }
    // }

    if (cfg->jumbo)
    {
#if RTE_VERSION < RTE_VERSION_NUM(21, 11, 0, 0)
        g_port_conf.rxmode.offloads |= DEV_RX_OFFLOAD_JUMBO_FRAME;
        g_port_conf.rxmode.max_rx_pkt_len = JUMBO_FRAME_SIZE(g_config.jumbo_mtu);
#else
        g_port_conf.rxmode.mtu = cfg->jumbo_mtu;
#endif
    }

    if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_IPV4_CKSUM)
    {
        g_port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_IPV4_CKSUM;
        g_dev_tx_offload_ipv4_cksum = 1;
    }
    else
    {
        g_dev_tx_offload_ipv4_cksum = 0;
    }

    if ((dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_TCP_CKSUM) &&
        (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_UDP_CKSUM))
    {
        g_dev_tx_offload_tcpudp_cksum = 1;
        g_port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_TCP_CKSUM | RTE_ETH_TX_OFFLOAD_UDP_CKSUM;
    }
    else
    {
        g_dev_tx_offload_tcpudp_cksum = 0;
    }

    if ((queue_num > (dev_info.max_rx_queues)) || (queue_num > (dev_info.max_tx_queues)))
    {
        printf("bad queue_num %d max rx %d max tx %d\n", queue_num, dev_info.max_rx_queues, dev_info.max_tx_queues);
        return -1;
    }

    if (rte_eth_dev_configure(port_id, queue_num, queue_num, &g_port_conf) < 0)
    {
        printf("dev configure fail\n");
        return -1;
    }

    for (i = 0; i < queue_num; i++)
    {
        if (port_init_rx(port, i, nb_rxd) < 0)
        {
            printf("init rx fail\n");
            return -1;
        }

        if (port_init_tx(port_id, i, nb_txd) < 0)
        {
            printf("init tx fail\n");
            return -1;
        }
    }

    return 0;
}

static int port_init_port_id(struct netif_port *port)
{
    int i = 0;
    uint16_t port_id = 0;
    char *pci = NULL;
    int socket = 0;

    for (i = 0; i < port->pci_num; i++)
    {
        pci = port->pci_list[i];
        if (rte_eth_dev_get_port_by_name(pci, &port_id) != 0)
        {
            printf("warning: cannot find port id by pci %s\n", pci);
            port_id = (uint16_t)i;
        }

        port->port_id_list[i] = port_id;
        socket = rte_eth_dev_socket_id(port_id);
        if (i == 0)
        {
            port->socket = socket;
        }
        else if (port->socket != socket)
        {
            printf("slaves not in same socket\n ");
            return -1;
        }
    }

    // if (port->bond) {
    //     if ((port->id = bond_create(port)) < 0) {
    //         return -1;
    //     }
    // } else {
    port->id = port->port_id_list[0];
    // }

    return 0;
}

static int port_init(dpdk_config *cfg, netif_port *port)
{
    if (port_init_port_id(port) < 0)
    {
        return -1;
    }

    if (port_init_mbuf_pool(cfg, port) < 0)
    {
        printf("port %s init mbuf pool error\n", port->pci);
        return -1;
    }

    // if (port->bond) {
    //     if (bond_config_slaves(port) < 0) {
    //         printf("bond init slaves error\n");
    //         return -1;
    //     }
    // }

    return port_config(cfg, port);
}

int port_init_all(struct dpdk_config *cfg)
{
    int nb_ports = 0;
    struct netif_port *port = NULL;
#if RTE_VERSION >= RTE_VERSION_NUM(18, 11, 0, 0)
    nb_ports = rte_eth_dev_count_avail();
#else
    nb_ports = rte_eth_dev_count();
#endif

    if ((nb_ports == 0) || (nb_ports < cfg->num_ports))
    {
        printf("not enough ports available: avail %d require %d\n", nb_ports, cfg->num_ports);
        return -1;
    }

    config_for_each_port(cfg, port)
    {
        if (port_init(cfg, port) < 0)
        {
            return -1;
        }
    }

    return 0;
}

struct rte_mempool *port_get_mbuf_pool(struct netif_port *p, int queue_id)
{
    if (queue_id < p->queue_num)
    {
        return p->mbuf_pool[queue_id];
    }

    return NULL;
}

static int port_start(struct netif_port *port)
{
    int ret = 0;
    int port_id = port->id;

    ret = rte_eth_dev_start(port_id);
    if (ret == 0)
    {
        rte_eth_macaddr_get(port_id, (struct rte_ether_addr *)&port->local_mac);

        // if (port->bond) {
        //     if (bond_wait(port) < 0) {
        //         return -1;
        //     }
        // }

        rte_eth_allmulticast_enable(port_id);
        rte_eth_promiscuous_enable(port_id);
        rte_eth_stats_reset(port_id);

        return 0;
    }
    else
    {
        printf("port start error: %s\n", rte_strerror(rte_errno));
    }

    return -1;
}

int port_start_all(struct dpdk_config *cfg)
{
    struct netif_port *port = NULL;

    config_for_each_port(cfg, port)
    {
        if (port_start(port) < 0)
        {
            return -1;
        }
    }

    return 0;
}