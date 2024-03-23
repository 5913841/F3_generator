#include "timer/timer.h"
#include <sys/time.h>
#include <time.h>

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

template <typename SpecificTimer>
Timer<SpecificTimer>::Timer(uint64_t ts_msec)
{
    this->ts_msec = ts_msec;
    this->next = nullptr;
}

template <typename SpecificTimer>
Timer<SpecificTimer>::Timer()
{
    this->ts_msec = current_ts_msec();
    this->next = nullptr;
}

template <typename SpecificTimer>
bool Timer<SpecificTimer>::operator<(const Timer<SpecificTimer> &other) const
{
    // smallest ts first
    return this->ts_msec > other.ts_msec;
}

template <typename SpecificTimer>
TimerList<SpecificTimer>::TimerList()
{
    head = new Timer(0);
    tail = head;
}

template <typename SpecificTimer>
TimerList<SpecificTimer>::TimerList(Timer<SpecificTimer> *hd)
{
    head = hd;
    tail = head;
}

template <typename SpecificTimer>
void TimerList<SpecificTimer>::trigger()
{
    uint64_t cur_ts = current_ts_msec();
    while (head != tail)
    {
        auto timer = head->next;
        if (timer->ts_msec + SpecificTimer::delay > cur_ts)
        {
            break;
        }
        int res = timer->callback();
        if (res >= 0)
        {
            add_job(timer);
        }
        head->next = timer->next;
        delete timer;
    }
}

template <typename SpecificTimer>
void TimerList<SpecificTimer>::add_job(SpecificTimer *tm)
{
    tm->ts_msec = current_ts_msec()
    tail->next = tm;
    tail = tm;
}
