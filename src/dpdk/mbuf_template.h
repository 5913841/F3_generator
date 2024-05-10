#ifndef _MBUF_TEMPLATE_H_
#define _MBUF_TEMPLATE_H_

#include <stdint.h>
#include <stdbool.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_version.h>
#include "dpdk/mbuf.h"
#include "common/define.h"

struct Socket;

struct mbuf_data
{
    uint8_t data[MBUF_DATA_SIZE];
    uint16_t l2_len;
    uint16_t l3_len;
    uint16_t l4_len;
    uint16_t l5_len;
    uint16_t data_len;
    uint16_t total_len;
};

// typedef int(*constructor)(Socket*, rte_mbuf*);
typedef std::function<int(Socket*, rte_mbuf*)> constructor;

struct mbuf_cache
{
    struct rte_mempool *mbuf_pool;
    struct mbuf_data data;
};

static inline void mbuf_set_userdata(struct rte_mbuf *m, void *data)
{
#if RTE_VERSION >= RTE_VERSION_NUM(20, 0, 0, 0)
    ((uint64_t *)(m->dynfield1))[0] = (uint64_t)data;
#else
    m->userdata = data;
#endif
}

static inline void *mbuf_get_userdata(struct rte_mbuf *m)
{
#if RTE_VERSION >= RTE_VERSION_NUM(20, 0, 0, 0)
    return (void *)(((uint64_t *)(m->dynfield1))[0]);
#else
    return m->userdata;
#endif
}

static inline struct rte_mbuf *mbuf_cache_alloc(struct mbuf_cache *p)
{
    uint8_t *data = NULL;

    rte_mbuf *m = rte_pktmbuf_alloc(p->mbuf_pool);
    if (unlikely(m == NULL))
    {
        return NULL;
    }

    if (likely(mbuf_get_userdata(m) == p))
    {
        mbuf_push_data(m, p->data.total_len);
    }
    else
    {
        mbuf_set_userdata(m, p);
        data = mbuf_push_data(m, p->data.total_len);
        memcpy(data, p->data.data, p->data.total_len);
    }

    // data = mbuf_push_data(m, p->data.total_len);
    // memcpy(data, p->data.data, p->data.total_len);

    return m;
}

int mbuf_template_pool_setby_constructors(mbuf_cache *mp, Socket* socket, constructor* constructors, const void *data, size_t len);

int mbuf_template_pool_setby_packet(mbuf_cache *mp, rte_mbuf *pkt);

#endif /* _MBUF_TEMPLATE_H_ */