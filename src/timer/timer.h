#ifndef __TIMERS_H__
#define __TIMERS_H__

#include <functional>
#include <queue>
#include <stdint.h>
#include <vector>

// return -1 means break
// return >=0 means schedule in res ms

// Return monotonic timestamp in msecs
uint64_t current_ts_msec();

// Return monotonic timestamp in usecs
uint64_t current_ts_usec();

class Timer
{
public:
    Timer();

    Timer(uint64_t ts_msec);

    static int id;

    // Timestamp in msecs
    uint64_t ts_msec;

    Timer *next;

    // Compare two timers
    bool operator<(const Timer &other) const;

    virtual int callback() { return 0; };
};

class SpecificTimers
{
    friend class TimersGroup;

private:
    Timer *head;
    Timer *tail;
    uint64_t delay;

public:
    SpecificTimers();
    SpecificTimers(Timer *head);

    // Trigger timers
    void trigger();

    // Add job to timers
    void add_job(Timer *tm, uint64_t ts_msec);

    // Schedule job to timers in msecs later
    void schedule_job(Timer *tm, uint64_t delay_msec);
};

struct TimersGroup
{
    std::vector<SpecificTimers> timers_group;

    TimersGroup(){};

    void add_timers(SpecificTimers timers);

    void add_job(Timer *tm, uint64_t ts_msec);

    void trigger();
};

extern __thread TimersGroup TIMERS;

#endif