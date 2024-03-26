#include "mbuf.h"

#include <rte_ethdev.h>
#include <pthread.h>
#include "dpdk/dpdk_config.h"
#include "port.h"

#define NB_MBUF (4096 * 8)

__thread struct mbuf_free_pool g_mbuf_free_pool;

struct rte_mempool *mbuf_pool_create(dpdk_config *cfg, const char *str, uint16_t port_id, uint16_t queue_id)
{
    int socket_id = 0;
    char name[RTE_RING_NAMESIZE];
    struct rte_mempool *mbuf_pool = NULL;
    int mbuf_size = 0;

    socket_id = rte_eth_dev_socket_id(port_id);
    snprintf(name, RTE_RING_NAMESIZE, "%s_%d_%d", str, port_id, queue_id);

    if (cfg->jumbo)
    {
        mbuf_size = JUMBO_MBUF_SIZE;
    }
    else
    {
        mbuf_size = RTE_MBUF_DEFAULT_BUF_SIZE;
    }

    mbuf_pool = rte_pktmbuf_pool_create(name, NB_MBUF,
                                        RTE_MEMPOOL_CACHE_MAX_SIZE, 0, mbuf_size, socket_id);

    if (mbuf_pool == NULL)
    {
        printf("rte_pkt_pool_create error\n");
    }
    return mbuf_pool;
}

static inline void mbuf_copy(struct rte_mbuf *dst, struct rte_mbuf *src)
{
    uint8_t *data = NULL;
    uint8_t *data2 = NULL;
    uint32_t len = 0;

    data = (uint8_t *)mbuf_eth_hdr(src);
    len = rte_pktmbuf_data_len(src);
    data2 = mbuf_push_data(dst, len);
    memcpy(data2, data, len);
}
