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


template <typename SpecificTimer>
class Timer
{
public:
    Timer();

    Timer(uint64_t ts_msec);

    static uint64_t delay;

    // Timestamp in msecs
    uint64_t ts_msec;

    Timer *next;

    // Compare two timers
    bool operator<(const Timer<SpecificTimer> &other) const;
    int callback(){};
};

template <typename SpecificTimer>
class TimerList
{
    friend class TimersGroup;

private:
    Timer<SpecificTimer> *head;
    Timer<SpecificTimer> *tail;

public:
    TimerList();
    TimerList(Timer<SpecificTimer> *head);


    // Trigger timers
    void trigger();

    // Add job to timers
    void add_job(SpecificTimer *tm);
};


#endif