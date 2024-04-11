// #include "packets.h"
// #include <rte_ethdev.h>
// #include "dpdk/mbuf.h"

// void tx_queue_flush(struct tx_queue *tq, uint16_t port_id, uint16_t queue_id)
// {

//     int i = 0;
//     int n = 0;
//     int num = 0;
//     struct rte_mbuf **tx = NULL;

//     if (tq->head == tq->tail)
//     {
//         return;
//     }

//     for (i = 0; i < 8; i++)
//     {
//         num = tq->tail - tq->head;
//         if (num > tq->tx_burst)
//         {
//             num = tq->tx_burst;
//         }

//         tx = &tq->tx[tq->head];
//         n = rte_eth_tx_burst(port_id, queue_id, tx, num);
//         tq->head += n;
//         if (tq->head == tq->tail)
//         {
//             tq->head = 0;
//             tq->tail = 0;
//             return;
//         }
//         else if (tq->tail < TX_QUEUE_SIZE)
//         {
//             return;
//         }
//     }

//     num = tq->tail - tq->head;
//     for (i = tq->head; i < tq->tail; i++)
//     {
//         rte_pktmbuf_free(tq->tx[i]);
//     }
//     tq->head = 0;
//     tq->tail = 0;
// }

// void send_packet_by_queue(struct tx_queue *tq, struct rte_mbuf *mbuf, uint16_t port_id, uint16_t queue_id)
// {
//     tq->tx[tq->tail] = mbuf;
//     tq->tail++;
//     if (((tq->tail - tq->head) >= tq->tx_burst) || (tq->tail == TX_QUEUE_SIZE))
//     {
//         tx_queue_flush(tq, port_id, queue_id);
//     }
// }

// rte_mbuf *recv_packet_by_queue(struct rx_queue *rq, uint16_t port_id, uint16_t queue_id)
// {

//     if (rq->head == rq->tail)
//     {
//         rq->head = rq->tail = 0;
//         int nb_rx = rte_eth_rx_burst(port_id, queue_id, rq->rx, RX_BURST_MAX);
//         if (nb_rx == 0)
//         {
//             return nullptr;
//         }
//         rq->tail += nb_rx;
//         // if (nb_rx > MBUF_PREFETCH_NUM)
//         // {
//         //     for (int i = 0; i < MBUF_PREFETCH_NUM; i++)
//         //     {
//         //         mbuf_prefetch(rq->rx[i]);
//         //     }
//         // }
//         // if (rq->head + MBUF_PREFETCH_NUM < rq->tail)
//         // {
//         //     mbuf_prefetch(rq->rx[rq->head + MBUF_PREFETCH_NUM]);
//         // }
//         rq->head += 1;
//         return rq->rx[0];
//     }
//     else
//     {
//         // if (rq->head + MBUF_PREFETCH_NUM < rq->tail)
//         // {
//         //     mbuf_prefetch(rq->rx[rq->head + MBUF_PREFETCH_NUM]);
//         // }
//         rq->head += 1;
//         return rq->rx[rq->head - 1];
//     }
// }