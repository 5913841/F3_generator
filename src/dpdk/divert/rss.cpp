#include "dpdk/divert/rss.h"
#include "common/define.h"
#include "dpdk_config.h"
#include "socket/socket.h"
#include <stdbool.h>
#include <rte_ethdev.h>
#include <rte_thash.h>

#define RSS_HASH_KEY_LENGTH 40
static uint8_t rss_hash_key_symmetric_be[RSS_HASH_KEY_LENGTH];
static uint8_t rss_hash_key_symmetric[RSS_HASH_KEY_LENGTH] = {
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
};

static uint64_t rss_get_rss_hf(struct rte_eth_dev_info *dev_info, uint8_t rss)
{
    uint64_t offloads = 0;
    uint64_t ipv4_flags = 0;
    uint64_t ipv6_flags = 0;

    offloads = dev_info->flow_type_rss_offloads;
    if (rss == RSS_L3) {
        ipv4_flags = RTE_ETH_RSS_IPV4 | RTE_ETH_RSS_FRAG_IPV4;
        ipv6_flags = RTE_ETH_RSS_IPV6 | RTE_ETH_RSS_FRAG_IPV6;
    } else if (rss == RSS_L3L4) {
        ipv4_flags = RTE_ETH_RSS_NONFRAG_IPV4_UDP | RTE_ETH_RSS_NONFRAG_IPV4_TCP;
        ipv6_flags = RTE_ETH_RSS_NONFRAG_IPV6_UDP | RTE_ETH_RSS_NONFRAG_IPV6_TCP;
    }

    if (g_config.ipv6) {
        if ((offloads & ipv6_flags) == 0) {
            return 0;
        }
    } else {
        if ((offloads & ipv4_flags) == 0) {
            return 0;
        }
    }

    return (offloads & (ipv4_flags | ipv6_flags));
}

int rss_config_port(struct rte_eth_conf *conf, struct rte_eth_dev_info *dev_info)
{
    uint64_t rss_hf = 0;
    struct rte_eth_rss_conf *rss_conf = NULL;

    /* no need to configure hardware */
    if (g_config.flood) {
        return 0;
    }

    rss_conf = &conf->rx_adv_conf.rss_conf;
    if (g_config.rss == RSS_AUTO) {
        if (g_config.mq_rx_rss) {
            conf->rxmode.mq_mode = RTE_ETH_MQ_RX_RSS;
            rss_conf->rss_hf = rss_get_rss_hf(dev_info, g_config.rss_auto);
        }
        return 0;
    }

    rss_hf = rss_get_rss_hf(dev_info, g_config.rss);
    if (rss_hf == 0) {
        return -1;
    }

    conf->rxmode.mq_mode = RTE_ETH_MQ_RX_RSS;
    rss_conf->rss_key = rss_hash_key_symmetric;
    rss_conf->rss_key_len = RSS_HASH_KEY_LENGTH,
    rss_conf->rss_hf = rss_hf;

    return 0;
}

static uint32_t rss_hash_data(uint32_t *data, int len)
{
    int i = 0;

    for (i = 0; i < len; i++) {
        data[i] = rte_be_to_cpu_32(data[i]);
    }

    return rte_softrss_be(data, len, rss_hash_key_symmetric_be);
}

// bool rss_check_socket(dpdk_config* cfg, struct Socket* sk)
// {
//     uint32_t hash = 0;

//     hash = FiveTuples::hash(FiveTuples(sk));
//     hash = hash % cfg->port->queue_num;
//     if (hash == cfg->queue_id) {
//         return true;
//     }

//     return false;
// }

void rss_init(void)
{
    rte_convert_rss_key((const uint32_t *)rss_hash_key_symmetric,
                        (uint32_t *)rss_hash_key_symmetric_be, RSS_HASH_KEY_LENGTH);
}
