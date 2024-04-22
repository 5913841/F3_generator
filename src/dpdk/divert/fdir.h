#ifndef __FDIR_H
#define __FDIR_H

struct config;
int fdir_init(struct dpdk_config *cfg);
void fdir_flush(struct dpdk_config *cfg);

#endif