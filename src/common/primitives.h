#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "dpdk/divert/rss.h"
#include "protocols/conf_protocol.h"

namespace primitives {

typedef void(*random_socket_t)(Socket* socket, void* data);

typedef void(*update_speed_t)(launch_control* lc, void* data);

extern dpdk_config * dpdk_config_pri;

extern std::vector<protocol_config> p_configs;

extern std::vector<Socket> sockets_ready_to_add;

extern thread_local Socket* socket_partby_pattern;

extern thread_local int socketsize_partby_pattern[MAX_PATTERNS];

extern thread_local int socketpointer_partby_pattern[MAX_PATTERNS];

extern random_socket_t random_methods[MAX_PATTERNS];

extern update_speed_t update_speed_methods[MAX_PATTERNS];

int get_index(int pattern, int index);

void set_configs_and_init(dpdk_config_user& usrconfig, char** argv); // Use dpdk config data structure to init the dpdk environment.

void set_pattern_num(int pattern_num); // Set the total pattern number used.

void add_pattern(protocol_config& p_config); // Add a manner pattern defined by protocol config data structure.

//should be use in preset mode
void add_fivetuples(const FiveTuples& five_tuples, int pattern); // Set a five-tuples use a specific pattern to generte flow.

void add_fivetuples(const Socket& socket, int pattern); // Set a five-tuples use a specific pattern to generte flow.

//should be use in non-preset mode
void set_random_method(random_socket_t random_method, int pattern_num, void* data); // Set specific manner pattern use a specific method to  generate five-tuples

void set_update_speed_method(update_speed_t update_speed_method, int pattern_num, void* data); // Set specific manner pattern use a specific method to control the speed.

void set_total_time(std::string total_time); // Set the total time the traffic sustained.

void run_generate(); // Start generation.


}

#endif // PRIMITIVES_H