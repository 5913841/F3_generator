#ifndef UNIQUE_TIMER_H
#define UNIQUE_TIMER_H

#include <cstdint>
#include <cstddef>
#include "dpdk/dpdk_config.h"
#include "common/define.h"
struct UniqueTimer {
    struct UniqueTimer *next;
    struct UniqueTimer *prev;
    uint64_t timer_tsc;
    // void* data;
};

struct UniqueTimerQueue {
    struct UniqueTimer head;
    uint64_t delay_tsc;
    virtual void callback(UniqueTimer* timer) = 0;
};

inline void unique_timer_init(struct UniqueTimer* timer)
{
    timer->next = timer;
    timer->prev = timer;
}

inline void unique_queue_init(struct UniqueTimerQueue *queue)
{
    unique_timer_init(&(queue->head));
}

inline void unique_queue_del(struct UniqueTimer* timer)
{
    struct UniqueTimer *prev = timer->prev;
    struct UniqueTimer *next = timer->next;

    if (timer != next) {
        prev->next = next;
        next->prev = prev;
        unique_timer_init(timer);
    }
}

inline void unique_queue_push(struct UniqueTimerQueue *queue, struct UniqueTimer* timer)
{
    struct UniqueTimer *head = &queue->head;
    struct UniqueTimer *next = head;
    struct UniqueTimer *prev = next->prev;

    /* [head->prev] [new-node] [head]  */
    timer->next = next;
    timer->prev = prev;
    next->prev = timer;
    prev->next = timer;
}

inline void unique_queue_add(struct UniqueTimerQueue *queue, struct UniqueTimer* timer, uint64_t now_tsc)
{
    timer->timer_tsc = now_tsc;
    unique_queue_del(timer);
    unique_queue_push(queue, timer);

}

inline UniqueTimer *socket_queue_first(struct UniqueTimerQueue *queue)
{
    struct UniqueTimer *head = &queue->head;

    if (head->next != head) {
        return head->next;
    }

    return NULL;
}

inline void unique_queue_trigger(struct UniqueTimerQueue *queue, uint64_t now_tsc)
{
    struct UniqueTimer *timer;
    while((timer = socket_queue_first(queue)) != NULL)
    {
        if(timer->timer_tsc + queue->delay_tsc <= now_tsc){
            unique_queue_del(timer);
            queue->callback(timer);
        } else {
            break;
        }
    }
}

struct UniqueQueueGroup
{
    struct UniqueTimerQueue* queues[MAX_UNIQUE_TIMER_SIZE];
    uint32_t size;

    UniqueQueueGroup()
    {
        size = 0;
        #pragma unroll
        for(int i = 0; i < MAX_UNIQUE_TIMER_SIZE; i++)
        {
            queues[i] = NULL;
        }
    }

    void add_queue(struct UniqueTimerQueue* queue)
    {
        if (size < MAX_UNIQUE_TIMER_SIZE)
        {
            queues[size++] = queue;
        }
    }
    
    void trigger()
    {
        uint64_t now_tsc = time_in_config();
        #pragma unroll
        for(int i = 0; i < MAX_UNIQUE_TIMER_SIZE; i++)
        {
            if(!queues[i]) break;
            unique_queue_trigger(queues[i], now_tsc);
        }
    }
};

extern thread_local UniqueQueueGroup TIMERS;

#endif