#ifndef __MBUF_H
#define __MBUF_H

#include <rte_mbuf.h>
#include <rte_arp.h>
#include <pthread.h>

#include "dpdk/port.h"

#define mbuf_eth_hdr(m) rte_pktmbuf_mtod(m, struct eth_hdr *)
#define RTE_PKTMBUF_PUSH(m, type) (type *)rte_pktmbuf_append(m, sizeof(type))
#define mbuf_push_eth_hdr(m) RTE_PKTMBUF_PUSH(m, struct eth_hdr)
#define mbuf_push_data(m, size) (uint8_t *)rte_pktmbuf_append(m, (size))

static inline void mbuf_prefetch(struct rte_mbuf *m)
{
    rte_prefetch0(mbuf_eth_hdr(m));
}

void mbuf_print(struct rte_mbuf *m, const char *tag);
void mbuf_log(struct rte_mbuf *m, const char *tag);

#define MBUF_LOG(m, tag)

int mbuf_pool_init(struct config *cfg);
struct rte_mempool *mbuf_pool_create(dpdk_config *cfg, const char *str, uint16_t port_id, uint16_t queue_id);

struct mbuf_free_pool
{
    int num;
    struct rte_mbuf *head;
};

extern __thread struct mbuf_free_pool g_mbuf_free_pool;

#define mbuf_free(m) rte_pktmbuf_free(m)

static void mbuf_free2(struct rte_mbuf *m)
{
    if (m)
    {
        m->next = g_mbuf_free_pool.head;
        g_mbuf_free_pool.head = m;
        g_mbuf_free_pool.num++;
        if (g_mbuf_free_pool.num >= 128)
        {
            rte_pktmbuf_free(m);
            g_mbuf_free_pool.head = NULL;
            g_mbuf_free_pool.num = 0;
        }
    }
}

#endif
