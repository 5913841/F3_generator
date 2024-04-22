#ifndef __RSS_H
#define __RSS_H

#include <stdbool.h>
#include <rte_ethdev.h>
#include "socket/socket.h"

struct socket;
struct work_space;
int rss_config_port(struct rte_eth_conf *conf, struct rte_eth_dev_info *dev_info);
bool rss_check_socket(Socket *sk);
void rss_init(void);

#endif