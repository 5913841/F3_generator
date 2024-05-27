#ifndef __CTL_H
#define __CTL_H

#include <pthread.h>

#include "dpdk/dpdk_config.h"

int ctl_thread_start(struct dpdk_config *cfg, pthread_t *thread);
void ctl_thread_wait(pthread_t thread);

#endif