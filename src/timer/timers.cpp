#include "timer/timer.h"
#include <sys/time.h>
#include <time.h>

thread_local TimersGroup TIMERS;

uint64_t current_ts_msec()
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec * 1000 + tp.tv_nsec / 1000 / 1000;
}

uint64_t current_ts_usec()
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec * 1000 * 1000 + tp.tv_nsec / 1000;
}

Timer::Timer(uint64_t ts_msec)
{
    this->ts_msec = ts_msec;
    this->next = nullptr;
}

Timer::Timer()
{
    this->ts_msec = current_ts_msec();
    this->next = nullptr;
}

bool Timer::operator<(const Timer &other) const
{
    // smallest ts first
    return this->ts_msec > other.ts_msec;
}

SpecificTimers::SpecificTimers()
{
    head = new Timer(0);
    tail = head;
}

SpecificTimers::SpecificTimers(Timer *hd)
{
    head = hd;
    tail = head;
}

void SpecificTimers::trigger()
{
    uint64_t cur_ts = current_ts_msec();
    while (head != tail)
    {
        auto timer = head->next;
        if (timer->ts_msec + delay > cur_ts)
        {
            break;
        }
        int res = timer->callback();
        if (res >= 0)
        {
            schedule_job(timer, res);
        }
        head->next = timer->next;
        delete timer;
    }
}

void SpecificTimers::add_job(Timer *tm, uint64_t ts_msec)
{
    tail->next = tm;
    tail = tm;
}

void SpecificTimers::schedule_job(Timer *tm, uint64_t delay_msec)
{
    tail->next = tm;
    tail = tm;
}

void TimersGroup::add_timers(SpecificTimers timers)
{
    timers.head->id = timers_group.size();
    timers_group.push_back(timers);
}

void TimersGroup::add_job(Timer *tm, uint64_t ts_msec)
{
    timers_group[tm->id].add_job(tm, ts_msec);
}

void TimersGroup::trigger()
{
    for (auto &timers : timers_group)
    {
        timers.trigger();
    }
}