#include "dpdk/mbuf_template.h"



int mbuf_template_pool_setby_constructors(mbuf_cache *mp, Socket* socket, constructor* constructors, const void *data, size_t len)
{
    rte_mbuf *m = rte_pktmbuf_alloc(mp->mbuf_pool);
    rte_pktmbuf_append(m, len);
    if (data != nullptr)
        memcpy(rte_pktmbuf_mtod(m, void *), data, len);
    mp->data.data_len = len;
    for (int i = 3; i >= 0; i--)
    {
        if (constructors[i] == nullptr)
            continue;
        int h_len = constructors[i](socket, m);
        len += h_len;
        if (i == SOCKET_L2)
        {
            mp->data.l2_len = h_len;
        }
        else if (i == SOCKET_L3)
        {
            mp->data.l3_len = h_len;
        }
        else if (i == SOCKET_L4)
        {
            mp->data.l4_len = h_len;
        }
        else if (i == SOCKET_L5)
        {
            mp->data.l5_len = h_len;
        }
    }
    mp->data.total_len = len;
    memcpy(mp->data.data, rte_pktmbuf_mtod(m, void *), rte_pktmbuf_pkt_len(m));
    mbuf_free2(m);
    return 0;
}

int mbuf_template_pool_setby_packet(mbuf_cache *mp, rte_mbuf *pkt)
{
    mp->data.data_len = rte_pktmbuf_data_len(pkt);
    mp->data.total_len = rte_pktmbuf_pkt_len(pkt);
    memcpy(mp->data.data, rte_pktmbuf_mtod(pkt, void *), rte_pktmbuf_pkt_len(pkt));
    return 0;
}