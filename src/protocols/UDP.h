#ifndef __UDP_H
#define __UDP_H

#include <netinet/udp.h>
#include <rte_mbuf.h>
#include <rte_common.h>

struct work_space;
struct config;
void udp_set_payload(int page_size);
int udp_init();
void udp_drop(struct rte_mbuf *m);

#endif