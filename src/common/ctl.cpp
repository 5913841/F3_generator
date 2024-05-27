#include <stdio.h>
#include <signal.h>
#include <stdbool.h>

#include "common/ctl.h"
#include "panel/panel.h"
#include "protocols/global_states.h"

#define CTL_CLIENT_LOG LOG_DIR"/F3_generator-ctl-client.log"
#define CTL_SERVER_LOG LOG_DIR"/F3_generator-ctl-server.log"

static bool g_stop = false;

static struct timeval g_last_tv;
static void ctl_wait_init(void)
{
    tick_wait_init(&g_last_tv);
}

static void ctl_wait_1s(void)
{
    tick_wait_one_second(&g_last_tv);
}

static void ctl_slow_start(FILE *fp, int *seconds)
{
    uint64_t launch_interval = 0;
    uint64_t step = 0;
    uint64_t cps = 0;
    uint32_t i = 0;

    uint32_t max_slow_start = 0;
    for (int i = 0; i < g_pattern_num; i++) {
        max_slow_start = max_slow_start > g_vars[i].slow_start ? max_slow_start : g_vars[i].slow_start;
    }

    for (i = 0; i < max_slow_start; i++) {
        if (g_stop) {
            break;
        }
        #pragma unroll
        for(int pattern = 0; pattern < MAX_PATTERNS; pattern++) {
            if(pattern >= g_pattern_num) break;
            int launch_num = g_vars[pattern].launch_num;
            int slow_start = g_vars[pattern].slow_start;
            if (i > slow_start || slow_start == 0)
            {
                continue;
            }
            step = (g_vars[pattern].cps / g_config->num_lcores) / slow_start;
            if (step <= 0) {
                step = 1;
            }
            cps = step * i;
            if(cps == 0) continue;
            launch_interval = (g_tsc_per_second * launch_num) / cps;
            #pragma unroll
            for(int j = 0; j < MAX_LCORES; j++){
            dpdk_config_percore* cfg_pc = g_config_percore_all[j];
            if(cfg_pc == NULL) break;
                launch_control *lc = &cfg_pc->launch_ctls[pattern];
                lc->launch_interval = launch_interval;
            }
        }
        ctl_wait_1s();
        net_stats_print_speed(fp, (*seconds)++);
    }
}


void wait_start(void)
{
    int i = 0;
    struct dpdk_config_percore *cp = NULL;
    int num = 0;
    int total = g_config->num_lcores;

    while (num != total) {
        num = 0;
        for (i = 0; i < total; i++) {
            cp = g_config_percore_all[i];
            if ((cp == NULL) || (!cp->start)) {
                usleep(1000);
                break;
            }
            num++;
        }
    }
}

void stop_all(void)
{
    return;
}

void exit_all(void)
{
    for (int i = 0; i < g_config->num_lcores; i++) {
        dpdk_config_percore *cp = g_config_percore_all[i];
        if (cp == NULL) {
            continue;
        }
        cp->start = false;
    }
}

static void *ctl_thread_main(void *data)
{
    int i = 0;
    int seconds = 0;
    int count = INT_MAX;
    FILE *fp = NULL;
    struct dpdk_config *cfg = NULL;

    cfg = (struct dpdk_config *)data;
    wait_start();

    ctl_wait_init();
    ctl_slow_start(fp, &seconds);

    for (i = 0; i < count; i++) {
        ctl_wait_1s();
        net_stats_print_speed(fp, seconds++);
        if (g_stop) {
            break;
        }
    }

    // stop_all();
    // for (i = 0; i < DELAY_SEC; i++) {
    //     ctl_wait_1s();
    //     net_stats_print_speed(fp, seconds++);
    // }

    exit_all();
    sleep(1);
    net_stats_print_total(fp);

    return NULL;
}

static void ctl_signal_handler(int signum)
{
    if (signum == SIGINT) {
        g_stop = true;
    }
}

int ctl_thread_start(struct dpdk_config *cfg, pthread_t *thread)
{
    int ret = 0;

    signal(SIGINT, ctl_signal_handler);

    ret = pthread_create(thread, NULL, ctl_thread_main, (void*)cfg);
    if (ret < 0) {
        return -1;
    }

    return 0;
}

void ctl_thread_wait(pthread_t thread)
{
    pthread_join(thread, NULL);
}
