#ifndef GLOBAL_STATES_H
#define GLOBAL_STATES_H

#include <stdint.h>
#include <protocols/TCP.h>
#include <protocols/UDP.h>
#include <protocols/HTTP.h>

enum protocol_type {
    p_tcp,
    p_udp,
    p_tcp_syn,
    p_tcp_ack,
};

enum gen_type {
    default_gen,
    scanning_gen,
    pulse_gen,
};

struct global_states {
    protocol_type p_type;
    gen_type g_type;
    global_tcp_vars tcp_vars;
    global_udp_vars udp_vars;
    global_http_vars http_vars;
    uint32_t slow_start;
    uint64_t cps;
    uint32_t launch_num;
};

struct global_templates {
    struct global_tcp_templates tcp_templates;
    struct global_udp_templates udp_templates;
};

extern struct global_states g_vars[MAX_PATTERNS];
extern __thread struct global_templates g_templates[MAX_PATTERNS];
extern int g_pattern_num;

#endif