#ifndef __TIMERS_H__
#define __TIMERS_H__

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

void tick_init(int ticks_per_sec);
void tick_time_init(struct tick_time *tt);
void tick_wait_init(struct timeval *last_tv);
uint64_t tick_wait_one_second(struct timeval *last_tv);

struct TimerBase
{
    TimerBase();

    TimerBase(uint64_t begin_tsc);

    uint64_t begin_tsc;

    TimerBase *next;

    virtual int *id() { return nullptr; }
    virtual uint64_t delay_tsc() { return 0; }

    // return -1 means break
    // return >=0 means schedule in res ms
    virtual int callback() { return 0; };
    bool operator<(const TimerBase &other) const;
};

// Return monotonic timestamp in msecs
uint64_t current_ts_msec();

// Return monotonic timestamp in usecs
uint64_t current_ts_usec();

template <typename TimerType>
struct Timer : public TimerBase
{
public:
    Timer();

    Timer(uint64_t ts_msec);

    static int timer_id;

    int *id() override { return &timer_id; }
};

template <typename TimerType>
int Timer<TimerType>::timer_id;

template <typename TimerType>
Timer<TimerType>::Timer()
{
    TimerBase();
}

template <typename TimerType>
Timer<TimerType>::Timer(uint64_t begin_tsc) : TimerBase(begin_tsc)
{
}

class SpecificTimers
{
    friend class TimersGroup;

private:
    TimerBase *head;
    TimerBase *tail;
    uint64_t delay_tsc;

public:
    SpecificTimers();
    SpecificTimers(TimerBase *head);

    // Trigger timers
    void trigger();

    // Add job to timers
    void add_job(TimerBase *tm, uint64_t begin_tsc);

    // Schedule job to timers in msecs later
    void schedule_job(TimerBase *tm);
};

struct TimersGroup
{
    std::vector<SpecificTimers> timers_group;

    TimersGroup(){};

    void add_timers(SpecificTimers timers);

    void add_job(TimerBase *tm, uint64_t begin_tsc);

    void trigger();
};

extern __thread TimersGroup TIMERS;

#endif