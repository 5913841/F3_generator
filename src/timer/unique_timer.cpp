#include "timer/unique_timer.h"

thread_local UniqueQueueGroup TIMERS;

// inline void unique_timer_init(struct UniqueTimer* timer)
// {
//     timer->next = timer;
//     timer->prev = timer;
// }

// inline void unique_queue_init(struct UniqueTimerQueue *queue)
// {
// }

// inline void unique_queue_del(struct UniqueTimer* timer)
// {
//     timer->next->prev = timer->prev;
//     timer->prev->next = timer->next;
// }

// inline void unique_queue_push(struct UniqueTimerQueue *queue, struct UniqueTimer* timer)
// {
//     struct UniqueTimer *head = &queue->head;
//     struct UniqueTimer *next = head;
//     struct UniqueTimer *prev = next->prev;

//     /* [head->prev] [new-node] [head]  */
//     timer->next = next;
//     timer->prev = prev;
//     next->prev = timer;
//     prev->next = timer;
// }

// inline void unique_queue_add(struct UniqueTimerQueue *queue, struct UniqueTimer* timer, uint64_t now_tsc)
// {
//     timer->timer_tsc = now_tsc;
//     unique_queue_del(timer);
//     unique_queue_push(queue, timer);

// }

// inline void unique_queue_trigger(struct UniqueTimerQueue *queue, uint64_t now_tsc)
// {
//     struct UniqueTimer *timer;
//     while((timer = socket_queue_first(queue)) != NULL)
//     {
//         if(timer->timer_tsc + queue->delay_tsc <= now_tsc){
//             unique_queue_del(timer);
//             queue->callback(timer);
//         } else {
//             break;
//         }
//     }
// }