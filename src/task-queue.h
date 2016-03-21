#pragma once
#include <stdint.h>
#include <assert.h>
#include "stdatomic.h"


#if defined(_MSC_VER)
    #define AtomicCompareAndSwap(addr, desired, expected) _InterlockedCompareExchange64(addr, desired, expected)
#else
    #define AtomicCompareAndSwap(addr, desired, expected) __sync_val_compare_and_swap(addr, expected, desired)
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum {
    kMaxQueueSize = 1024,
    kQueueMask = kMaxQueueSize - 1,
};

typedef struct TaskQueue {
    struct Task*        data[kMaxQueueSize];
    volatile int64_t    top;
    volatile int64_t    bottom;
} TaskQueue;

/// @brief Returns a rough estimate of how many items are in the queue. Note that
///     this is only an estimate because other threads can change the size during
///     this call.
inline int64_t spmcqSize(TaskQueue const* queue)
{
    assert(queue);
    return queue->bottom - queue->top;
}

/// @brief Pushes a new item onto the bottom of the queue
/// @return 0 on success, 1 on failure (queue is full)
inline int spmcqPush(TaskQueue* queue, struct Task* value)
{
    assert(queue);
    if (spmcqSize(queue) == kMaxQueueSize) {
        return 1;
    }

    int64_t const bottom = queue->bottom;
    queue->data[bottom & kQueueMask] = value;

    std_atomic_signal_fence(memory_order_seq_cst);

    queue->bottom = bottom + 1;

    return 0;
}

inline Task* spmcqPop(TaskQueue* queue)
{
    assert(queue);

    int64_t bottom = queue->bottom - 1;
    queue->bottom = bottom;
    std_atomic_thread_fence(memory_order_seq_cst);
    int64_t top = queue->top;

    if (top <= bottom) {
        struct Task* value = queue->data[bottom & kQueueMask];
        if (top != bottom) {
            return value;
        }
        if (AtomicCompareAndSwap(&queue->top, top + 1, top) != top) {
            value = NULL;
        }
        queue->bottom = top + 1;
        return value;
    } else {
        queue->bottom = top;
        return NULL;
    }
}

inline Task* spmcqSteal(TaskQueue* queue)
{
    int64_t top = queue->top;
    std_atomic_signal_fence(memory_order_seq_cst);
    int64_t bottom = queue->bottom;

    if (top < bottom) {
        struct Task* value = queue->data[top & kQueueMask];
        if (AtomicCompareAndSwap(&queue->top, top + 1, top) != top) {
            return nullptr;
        }
        return value;
    } else {
        return nullptr;
    }
}

#ifdef __cplusplus
} // extern "C" {
#endif /* __cplusplus */
