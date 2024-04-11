#ifndef __TIMERS_H__
#define __TIMERS_H__

#include "timer/clock.h"

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

extern __thread TimersGroup CTIMERS;

#endif