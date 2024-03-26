#ifndef _PACKETS_H_
#define _PACKETS_H_

#include <stdint.h>
#include <rte_mbuf.h>
#include "common/define.h"

struct tx_queue
{
    uint16_t head;
    uint16_t tail;
    uint16_t tx_burst;
    struct rte_mbuf *tx[TX_QUEUE_SIZE];
};

struct rx_queue
{
    uint16_t head;
    uint16_t tail;
    uint16_t rx_burst;
    struct rte_mbuf *rx[RX_QUEUE_SIZE];
};

void tx_queue_flush(struct tx_queue *tq, uint16_t port_id, uint16_t queue_id);
void send_packet_by_queue(struct tx_queue *tq, struct rte_mbuf *mbuf, uint16_t port_id, uint16_t queue_id);
rte_mbuf *recv_packet_by_queue(struct rx_queue *rq, uint16_t port_id, uint16_t queue_id);

#endif /* _PACKETS_H_ */