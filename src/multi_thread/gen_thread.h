#ifndef __GEN_THREAD_H__
#define __GEN_THREAD_H__

#include <stdint.h>
#include "common/define.h"
#include "common/packets.h"

class GenThread {
public:
    uint8_t core_id;
    uint8_t port_id;
    uint8_t queue_id;
    struct netif_port *port;
    struct tx_queue mbuf_tx;
    struct rx_queue mbuf_rx;
    
};

#endif /* __GEN_THREAD_H__ */