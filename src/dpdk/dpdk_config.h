#ifndef DPDK_CONFIG_H_
#define DPDK_CONFIG_H_

#include "common/define.h"
#include "dpdk/port.h"
#include "dpdk/dpdk_config_user.h"
#include "common/utils.h"
#include "arpa/inet.h"

struct dpdk_config {
    int num_lcores;
    int lcores[MAX_LCORES];
    uint8_t rss;
    bool mq_rx_rss;
    uint8_t rss_auto;
    bool jumbo;
    uint16_t jumbo_mtu;
    uint16_t vlan_id;
    int num_rx_queues;
    int num_tx_queues;
    int num_ports;
    struct netif_port ports[NETIF_PORT_MAX];
    char socket_mem[RTE_ARG_LEN];
    dpdk_config(dpdk_config_user* user_config)
    {
        num_lcores = user_config->lcores.size();
        memcpy(lcores, user_config->lcores.data(), num_lcores * sizeof(int));
        num_ports = user_config->ports.size();
        for(int i=0;i<user_config->ports.size();i++)
        {
            memcpy(ports[i].pci, user_config->ports[i].data(), PCI_LEN * sizeof(char));
            ports[i].pci_num = 1;
        }
        for(int i=0;i<user_config->queue_num_per_port.size();i++)
        {
            ports[i].queue_num = user_config->queue_num_per_port[i];
        }
        for(int i=0;i<user_config->gateway_for_ports.size();i++)
        {
            std::string type = distinguish_address_type(user_config->gateway_for_ports[i]);
            if (type == "IPv4")
            {
                ports[i].gateway_ipv6 = false;
                inet_aton(user_config->gateway_for_ports[i].data(), &ports[i].gateway_ip.ip);
            }
            else if(type == "IPv6")
            {
                ports[i].gateway_ipv6 = true;
                inet_pton(AF_INET6, user_config->gateway_for_ports[i].data(), &ports[i].gateway_ip.in6);
            }
            else if(type == "MAC Address")
            {
                memcpy(&ports[i].gateway_mac, ether_aton(user_config->gateway_for_ports[i].data()), sizeof(ether_addr));
            }
            else
            {
                printf("Error: Invalid gateway address type\n");
                exit(1);
            }
        }
    }
};

#endif /* DPDK_CONFIG_H_ */