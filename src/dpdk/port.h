#ifndef _PORT_H_
#define _PORT_H_

#include <rte_ethdev.h>
#include <rte_kni.h>
#include <rte_mbuf.h>
#include <netinet/ether.h>
#include "common/define.h"
#include "dpdk/bond/bond.h"
#include "netinet/in.h"
#include "socket/socket.h"

struct netif_port {
    int id; /* DPDK port id */
    int queue_num;
    int socket; /* cpu socket */
    bool enable;
    bool gateway_ipv6;
    ipaddr_t gateway_ip;
    struct ether_addr gateway_mac;
    struct ether_addr local_mac;
    struct rte_mempool *mbuf_pool[MAX_LCORES];

    struct rte_kni *kni;
    struct vxlan *vxlan;

    /* bond */
    bool bond;
    uint8_t bond_mode;
    uint8_t bond_policy;
    uint8_t pci_num;
    char bond_name[BOND_NAME_MAX];

    union {
        char pci[PCI_LEN + 1];
        char pci_list[PCI_NUM_MAX][PCI_LEN + 1];
    };

    uint16_t port_id_list[PCI_NUM_MAX];
    netif_port(): id(-1), queue_num(-1), socket(-1), enable(false), gateway_ipv6(false), gateway_ip(0), gateway_mac(), local_mac(), kni(NULL), vxlan(NULL), bond(false), bond_mode(0), bond_policy(0), pci_num(1), bond_name(), pci(), port_id_list() {};
};


extern uint8_t g_dev_tx_offload_ipv4_cksum;
extern uint8_t g_dev_tx_offload_tcpudp_cksum;

int port_init_all(struct dpdk_config *cfg);
int port_start_all(struct dpdk_config *cfg);
void port_stop_all(struct dpdk_config *cfg);
void port_clear(uint16_t port_id, uint16_t queue_id);
int port_config(struct netif_port *port);
struct rte_mempool *port_get_mbuf_pool(struct netif_port *p, int queue_id);

#endif /* _PORT_H_ */

// dperf