#include "timer/timer.h"
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <dpdk/rte_cycles.h>
#include <string.h>
#include "dpdk/dpdk_config.h"

thread_local TimersGroup TIMERS;


static uint64_t g_tsc_per_tick = 0;
uint64_t g_tsc_per_second = 0;

void tick_wait_init(struct timeval *last_tv)
{
    gettimeofday(last_tv, NULL);
}

uint64_t tick_wait_one_second(struct timeval *last_tv)
{
    uint64_t us = 0;
    uint64_t us_wait = 0;
    uint64_t sec = 1000 * 1000;
    struct timeval tv;

    while(1) {
        gettimeofday(&tv, NULL);
        us = (tv.tv_sec - last_tv->tv_sec) * 1000 *1000 + tv.tv_usec - last_tv->tv_usec;
        if (us >= sec) {
            *last_tv = tv;
            break;
        }
        us_wait = sec - us;
        if (us_wait > 1000) {
            usleep(500);
        } else if (us_wait > 100) {
            usleep(50);
        }
    }

    return us;
}

static uint64_t tick_get_hz(void)
{
    int i = 0;
    uint64_t last_tsc = 0;
    uint64_t tsc = 0;
    struct timeval last_tv;
    uint64_t total = 0;
    int num = 4;

    tick_wait_init(&last_tv);
    last_tsc = rte_rdtsc();
    for (i = 0; i < num; i++) {
        tick_wait_one_second(&last_tv);
        tsc = rte_rdtsc();
        total += tsc - last_tsc;
        last_tsc = tsc;
    }

    return total / num;
}

void tick_init(int ticks_per_sec)
{
    uint64_t hz = 0;

    if (ticks_per_sec == 0) {
        ticks_per_sec = TICKS_PER_SEC_DEFAULT;
    }

    hz = tick_get_hz();
    g_tsc_per_second = hz;
    g_tsc_per_tick = hz / ticks_per_sec;
}

static void tsc_time_init(struct tsc_time *tt, uint64_t now, uint64_t interval)
{
    tt->last = now;
    tt->interval = interval;
    tt->count = 0;
}

void tick_time_init(struct tick_time *tt)
{
    uint64_t now = rte_rdtsc();

    memset(tt, 0, sizeof(struct tick_time));
    tsc_time_init(&tt->tick, now, g_tsc_per_tick);
    tsc_time_init(&tt->us, now,  g_tsc_per_second / 1000000);
    tsc_time_init(&tt->ms, now,  g_tsc_per_second / 1000);
    tsc_time_init(&tt->ms100, now,  g_tsc_per_second / 10);
    tsc_time_init(&tt->second, now, g_tsc_per_second);
}


uint64_t current_ts_msec()
{
    tsc_time ts = g_config_percore->time.ms;
    return ts.count + (rte_rdtsc() - ts.last) / ts.interval;
}

uint64_t current_ts_usec()
{
    tsc_time ts = g_config_percore->time.ms;
    return ts.count + (rte_rdtsc() - ts.last) / ts.interval;
}

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
    uint64_t cur_ts = current_ts_msec();
    while (head != tail)
    {
        auto timer = head->next;
        if (timer->begin_tsc + delay_tsc > cur_ts)
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