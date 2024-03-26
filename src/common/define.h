#ifndef __DEFINE_H__
#define __DEFINE_H__

#define RTE_ARG_LEN 64
#define CACHE_ALIGN_SIZE 64
#define TCP_WIN (1460 * 40)
#define NETWORK_PORT_NUM 65536

#define PACKET_SIZE_MAX 1514
#define DEFAULT_CPS 1000
#define DEFAULT_INTERVAL 1 /* 1s */
#define DEFAULT_DURATION 60
#define DEFAULT_TTL 64
#define ND_TTL 255
#define DEFAULT_LAUNCH 4
#define DELAY_SEC 4
#define WAIT_DEFAULT 3
#define SLOW_START_DEFAULT 30
#define SLOW_START_MIN 10
#define SLOW_START_MAX 600
#define KEEPALIVE_REQ_NUM 32767 /* 15 bits */

#define MAX_LCORES 64 // THREAD_NUM_MAX

#define ETH_ADDR_LEN 6
#define ETH_ADDR_STR_LEN 17

#define NETIF_PORT_MAX 4
#define PCI_LEN 12

#define NB_RXD 4096
#define NB_TXD 4096

#define RX_BURST_MAX NB_RXD
#define TX_QUEUE_SIZE NB_TXD
#define RX_QUEUE_SIZE NB_RXD
#define MBUF_PREFETCH_NUM 4

#define TX_BURST_MAX 1024
#define TX_BURST_DEFAULT 8

#define NB_RXD 4096
#define NB_TXD 4096

#define BOND_SLAVE_MAX 4
#define PCI_NUM_MAX BOND_SLAVE_MAX
#define BOND_NAME_MAX 32
#define BOND_WAIT_SEC 16

#define RSS_NONE 0
#define RSS_L3 1
#define RSS_L3L4 2
#define RSS_AUTO 3

#define KNI_NAMESIZE 10

#define VLAN_ID_MIN 1
#define VLAN_ID_MAX 4094

#define PIPELINE_MIN 0
#define PIPELINE_MAX 100
#define PIPELINE_DEFAULT 0

#define JUMBO_FRAME_SIZE(mtu) ((mtu) + 14 + ETHER_CRC_LEN)
#define JUMBO_PKT_SIZE(mtu) ((mtu) + 14)
#define JUMBO_MTU_MIN 9000
#define JUMBO_MTU_MAX 9710
#define JUMBO_MTU_DEFAULT JUMBO_MTU_MAX
#define JUMBO_MBUF_SIZE (1024 * 11)
#define MBUF_DATA_SIZE (1024 * 10)

#define SOCKET_L2 0
#define SOCKET_L3 1
#define SOCKET_L4 2
#define SOCKET_L5 3

#define RETRANSMIT_NUM_MAX 4
#define RETRANSMIT_TIMEOUT_SEC 2
// #define TICKS_PER_SEC_DEFAULT (10 * 1000)
// #define RETRANSMIT_TIMEOUT          (TSC_PER_SEC * RETRANSMIT_TIMEOUT_SEC)
// #define REQUEST_INTERVAL_DEFAULT    (TSC_PER_SEC * 60)
#define MS_PER_SEC (1000)
#define RETRANSMIT_TIMEOUT (MS_PER_SEC * RETRANSMIT_TIMEOUT_SEC)
#define REQUEST_INTERVAL_DEFAULT (MS_PER_SEC * 60)

#define HTTP_ACK_DELAY_MAX 1024
#define HTTP_F_CONTENT_LENGTH_AUTO 0x1
#define HTTP_F_CONTENT_LENGTH 0x2
#define HTTP_F_TRANSFER_ENCODING 0x4
#define HTTP_F_CLOSE 0x8
#define HTTP_HOST_MAX 128
#define HTTP_PATH_MAX 256
#define HTTP_HOST_DEFAULT "dperf"
#define HTTP_PATH_DEFAULT "/"

#define STRING_SIZE(str) (sizeof(str) - 1)

#define CONTENT_LENGTH_NAME "Content-Length:"
#define CONTENT_LENGTH_SIZE STRING_SIZE(CONTENT_LENGTH_NAME)

#define TRANSFER_ENCODING_NAME "Transfer-Encoding:"
#define TRANSFER_ENCODING_SIZE STRING_SIZE(TRANSFER_ENCODING_NAME)

#define CONNECTION_NAME "Connection:"
#define CONNECTION_SIZE STRING_SIZE(CONNECTION_NAME)

#define HTTP_PARSE_OK 0
#define HTTP_PARSE_END 1
#define HTTP_PARSE_ERR -1

#define SEND_WINDOW_MAX 16
#define DEFAULT_WSCALE 13

#define IP_FLAG_DF htons(0x4000)

#define SEND_WINDOW_MAX 16
#define SEND_WINDOW_MIN 2
#define SEND_WINDOW_DEFAULT 4

#define MSS_IPV4 (PACKET_SIZE_MAX - 14 - 20 - 20)
#define MSS_IPV6 (PACKET_SIZE_MAX - 14 - 40 - 20)

#define force_convert(var, type) (*((type *)(&(var))))

#define config_for_each_port(cfg, port) \
    for ((port) = &((cfg)->ports[0]); ((port) - &((cfg)->ports[0])) < (cfg)->num_ports; port++)

#define get_packet_begin(base, offset, type) ((type *)((void *)(base) + (offset) - (sizeof(type))))

#define MAX_PROTOCOLS_PER_LAYER 5

#define THREAD_NUM_MAX 64

#endif