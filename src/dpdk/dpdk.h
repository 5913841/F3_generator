#ifndef DPDK_H
#define DPDK_H

int dpdk_init(struct dpdk_config *cfg, char *argv0);
void dpdk_run(int (*lcore_main)(void*), void* data);

#endif