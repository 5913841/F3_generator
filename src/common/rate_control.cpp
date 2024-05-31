#include "common/rate_control.h"
#include "dpdk/dpdk_config.h"

uint64_t get_launch_num(launch_control* lc, int pattern)
{
    uint64_t launch_num = lc->launch_num;
    uint64_t num = launch_num;
    uint64_t gap = 0;
    uint64_t cc = lc->cc;

    if (lc->launch_next <= time_in_config()) {
        lc->launch_next += lc->launch_interval;
        if (cc > 0) {
            if (g_net_stats.socket_current[pattern] < cc) {
                gap = cc - g_net_stats.socket_current[pattern];
                if (gap < launch_num) {
                    num = gap;
                }
            } else {
                num = 0;
            }
        }

        return num;
    } else {
        return 0;
    }
}

uint64_t client_assign_task(uint64_t target)
{
    uint64_t val = 0;
    uint64_t id = (uint64_t)(g_config_percore->lcore_id);
    uint64_t cpu_num = (uint64_t)g_config->num_lcores;

    /* low target. Some CPUs idle. Some CPUs have only 1 task*/
    if (target <= cpu_num) {
        if (id < target) {
            val = 1;
        }
    } else {
        val = (uint64_t)(((double)(target)) / cpu_num);
        if (id == 0) {
            val = target - val * (cpu_num - 1);
        }
    }

    return val;
}


int launch_control_init(struct launch_control *lc, int total_cps, int total_cc, int launch_num)
{
    uint64_t cps = 0;
    uint64_t cc = 0;
    struct config *cfg = cfg;

    cps = client_assign_task(total_cps);
    cc = client_assign_task(total_cc);

    /* This is an idle CPU */
    // if (cps == 0) {
    //     return 0;
    // }

    lc->cc = cc;
    /* For small scale test, launch once a second */
    if (cps <= launch_num) {
        lc->launch_num = cps;
        lc->launch_interval = g_tsc_per_second;
    } else {
        /* For large-scale tests, multiple launches in one second */
        lc->launch_num = launch_num;
        lc->launch_interval = (g_tsc_per_second / (cps / lc->launch_num));
    }
    lc->launch_interval_default = lc->launch_interval;
    lc->launch_next = rte_rdtsc() + g_tsc_per_second * 1;

    return 0;
}