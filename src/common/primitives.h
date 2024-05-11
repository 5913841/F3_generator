#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "dpdk/divert/rss.h"
#include "protocols/conf_protocol.h"

namespace primitives {

typedef void(*random_socket_t)(Socket* socket);

extern dpdk_config * dpdk_config_pri;

extern std::vector<protocol_config> p_configs;

extern std::vector<Socket> sockets_ready_to_add;

extern thread_local Socket* socket_partby_pattern;

extern thread_local int socketsize_partby_pattern[MAX_PATTERNS];

extern thread_local int socketpointer_partby_pattern[MAX_PATTERNS];

extern random_socket_t random_methods[MAX_PATTERNS];

void set_configs_and_init(dpdk_config_user& usrconfig, char** argv);

void set_pattern_num(int pattern_num);

void add_pattern(protocol_config& p_config);

//should be use in preset mode
void add_fivetuples(FiveTuples& five_tuples, int pattern);

//should be use in non-preset mode
void set_random_method(random_socket_t random_method, int pattern_num);

void set_total_time(std::string total_time);

void run_setted();


}

#endif // PRIMITIVES_H