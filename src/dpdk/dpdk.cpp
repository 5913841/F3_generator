#include "dpdk.h"
#include "dpdk_config.h"
#include "dpdk/divert/fdir.h"
#include "dpdk/divert/rss.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_prefetch.h>
#include <rte_mbuf.h>
#include <rte_pdump.h>
#include <rte_tailq.h>
#include <rte_version.h>

static void dpdk_set_lcores(struct dpdk_config *cfg, char *lcores_argv)
{
    int i = 0;
    char lcore_buf[64];

    for (i = 0; i < cfg->num_lcores; i++)
    {
        if (i == 0)
        {
            sprintf(lcore_buf, "%d@(%d)", i, cfg->lcores[i]);
        }
        else
        {
            sprintf(lcore_buf, ",%d@(%d)", i, cfg->lcores[i]);
        }
        strcat(lcores_argv, lcore_buf);
    }
}

static int dpdk_append_pci(struct dpdk_config *cfg, int argc, char *argv[], char *flag_pci)
{
    int i = 0;
    int num = 0;
    struct netif_port *port = NULL;

    config_for_each_port(cfg, port)
    {
        for (i = 0; i < port->pci_num; i++)
        {
            argv[argc] = flag_pci;
            argv[argc + 1] = port->pci_list[i];
            argc += 2;
            num += 2;
        }
    }

    return num;
}

static int dpdk_set_socket_mem(struct dpdk_config *cfg, char *socket_mem, char *file_prefix)
{
    int size = 0;

    if (strlen(cfg->socket_mem) <= 0)
    {
        return 0;
    }

    size = snprintf(socket_mem, RTE_ARG_LEN, "--socket-mem=%s", cfg->socket_mem);
    if (size >= RTE_ARG_LEN)
    {
        return -1;
    }

    size = snprintf(file_prefix, RTE_ARG_LEN, "--file-prefix=%d", getpid());
    if (size >= RTE_ARG_LEN)
    {
        return -1;
    }

    return 0;
}

int dpdk_eal_init(struct dpdk_config *cfg, char *argv0)
{
    int argc = 4;
    char lcores_argv[2048] = "--lcores=";
#if RTE_VERSION >= RTE_VERSION_NUM(20, 0, 0, 0)
    char flag_pci[] = "-a";
#else
    char flag_pci[] = "-w";
#endif
    char socket_mem[64] = "";
    char file_prefix[64] = "";
    char *argv[4 + (NETIF_PORT_MAX * PCI_NUM_MAX * 2)] = {argv0, lcores_argv, socket_mem, file_prefix, NULL};

    if (dpdk_set_socket_mem(cfg, socket_mem, file_prefix) < 0)
    {
        printf("dpdk_set_socket_mem fail\n");
        return -1;
    }

    dpdk_set_lcores(cfg, lcores_argv);
    argc += dpdk_append_pci(cfg, argc, argv, flag_pci);

    if (rte_eal_init(argc, argv) < 0)
    {
        printf("rte_eal_init fail\n");
        return -1;
    }

    return 0;
}

int dpdk_init(struct dpdk_config *cfg, char *argv0)
{
    if (dpdk_eal_init(cfg, argv0) < 0)
    {
        printf("dpdk_eal_init fail\n");
        return -1;
    }

    if (port_init_all(cfg) < 0)
    {
        printf("port init fail\n");
        return -1;
    }

    rte_pdump_init();

    if (port_start_all(cfg) < 0)
    {
        printf("start port fail\n");
        return -1;
    }

    if(cfg->num_lcores == 1){
        g_config_percore = new dpdk_config_percore(cfg);
        g_config_percore_all[0] = g_config_percore;
        net_stats_init();
    }
    if(cfg->use_rss) rss_init();
    if(cfg->use_fdir) fdir_init(cfg);


    return 0;
    // if (kni_start() < 0) {
    //     printf("kni start fail\n");
    //     return -1;
    // }


    // tick_init(cfg->ticks_per_sec);
    // config_set_tsc(cfg, g_tsc_per_second);
}

struct dpdk_lcore_thread
{
    int (*lcore_main)(void*);
    void* data;
};

int dpdk_run_thread(void* thd)
{
    dpdk_lcore_thread* thread = (dpdk_lcore_thread*)thd;
    g_config_percore = new dpdk_config_percore(g_config);
    g_config_percore_all[g_config_percore->lcore_id] = g_config_percore;
    net_stats_init();
    return thread->lcore_main(thread->data);
}

void dpdk_run(int (*lcore_main)(void*), void* data)
{
    int lcore_id = 0;

    RTE_LCORE_FOREACH(lcore_id) {
        if (lcore_id == 0) {
            continue;
        }
        dpdk_lcore_thread* thd = new dpdk_lcore_thread();
        thd->lcore_main = lcore_main;
        thd->data = data;
        rte_eal_remote_launch(dpdk_run_thread, (void*)thd, lcore_id);
    }
    
    g_config_percore = new dpdk_config_percore(g_config);
    g_config_percore_all[g_config_percore->lcore_id] = g_config_percore;
    net_stats_init();
    lcore_main(data);
    rte_eal_mp_wait_lcore();
}