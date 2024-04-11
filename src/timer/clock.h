#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <functional>
#include <queue>
#include <stdint.h>
#include <vector>
#include <rte_cycles.h>

#define TICKS_PER_SEC_DEFAULT (10 * 1000)
#define TSC_PER_SEC g_tsc_per_second

extern uint64_t g_tsc_per_second;

struct tsc_time
{
    uint64_t last;
    uint64_t count;
    uint64_t interval;
};

struct tick_time
{
    uint64_t tsc;
    struct tsc_time tick;
    struct tsc_time us;
    struct tsc_time ms;
    struct tsc_time ms100;
    struct tsc_time second;
};

static inline uint64_t tsc_time_go(struct tsc_time *tt, uint64_t tsc_now)
{
    uint64_t count = 0;
    while (tt->last + tt->interval <= tsc_now) {
        tt->count++;
        tt->last += tt->interval;
        count++;
    }
    return count;
}

static inline void tick_time_update(struct tick_time *tt)
{
    tt->tsc = rte_rdtsc();
}


// Return monotonic timestamp in msecs
uint64_t current_ts_msec();

// Return monotonic timestamp in usecs
uint64_t current_ts_usec();

void tick_init(int ticks_per_sec);
void tick_time_init(struct tick_time *tt);
void tick_wait_init(struct timeval *last_tv);
uint64_t tick_wait_one_second(struct timeval *last_tv);

#endif // __CLOCK_H__