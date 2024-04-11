#include "timer/timer.h"
#include "dpdk/dpdk_config.h"

thread_local TimersGroup CTIMERS;


TimerBase::TimerBase()
{
    this->begin_tsc = rte_rdtsc();
    this->next = nullptr;
}

TimerBase::TimerBase(uint64_t ts_msec)
{
    this->begin_tsc = ts_msec;
    this->next = nullptr;
}

bool TimerBase::operator<(const TimerBase &other) const
{
    // smallest ts first
    return this->begin_tsc > other.begin_tsc;
}

SpecificTimers::SpecificTimers()
{
    head = new TimerBase(0);
    tail = head;
}

SpecificTimers::SpecificTimers(TimerBase *hd)
{
    head = hd;
    tail = head;
}

void SpecificTimers::trigger()
{
    uint64_t cur_ts = time_in_config();
    while (head->next!=nullptr)
    {
        auto timer = head->next;
        if (timer == nullptr || timer->begin_tsc + delay_tsc > cur_ts)
        {
            break;
        }
        int res = timer->callback();
        if (res >= 0)
        {
            schedule_job(timer);
        }
        head->next = timer->next;
        delete timer;
    }
    if (head->next == nullptr)
    {
        tail = head;
    }
}

void SpecificTimers::add_job(TimerBase *tm, uint64_t begin_tsc)
{
    tail->next = tm;
    tail = tm;
    tail->begin_tsc = begin_tsc;
}

void SpecificTimers::schedule_job(TimerBase *tm)
{
    tail->next = tm;
    tail = tm;
    tail->begin_tsc = rte_rdtsc();
}

void TimersGroup::add_timers(SpecificTimers timers)
{
    *timers.head->id() = timers_group.size();
    timers.delay_tsc = timers.head->delay_tsc();
    timers_group.push_back(timers);
}

void TimersGroup::add_job(TimerBase *tm, uint64_t ts_msec)
{
    timers_group[*tm->id()].add_job(tm, ts_msec);
}

void TimersGroup::trigger()
{
    for (auto &timers : timers_group)
    {
        timers.trigger();
    }
}