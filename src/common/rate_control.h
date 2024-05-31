#ifndef RATE_CONTROL_H
#define RATE_CONTROL_H

#include <stdint.h>

struct launch_control {
    uint64_t cc;
    uint64_t launch_next;
    uint64_t launch_interval;
    uint64_t launch_interval_default;
    uint32_t launch_num;
};

uint64_t get_launch_num(launch_control* lc, int pattern);

uint64_t client_assign_task(uint64_t target);


int launch_control_init(struct launch_control *lc, int total_cps, int total_cc, int launch_num);

#endif