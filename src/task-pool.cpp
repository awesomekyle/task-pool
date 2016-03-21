#include <stdlib.h>
#include <string.h>
#include <thread>
#include "task-pool/task-pool.h"
#include "task-queue.hpp"

#if defined(_MSC_VER)
    #include <intrin.h>
    #define ALIGN(x) alignas(x)
    #define AtomicAdd(val, add) _InterlockedExchangeAdd((volatile long*)val, add)
#elif defined(__GNUC__)
    #define ALIGN(x) alignas(x)
    #define AtomicAdd(val, add) __sync_add_and_fetch(val, add)
#endif

#if defined(__MACH__)
    #include <x86intrin.h>
    #define thread_local __thread
#endif // defined(__MACH__)

/* constants */
#define CACHE_LINE_SIZE 64
enum {
    kMaxTasks = 1024,
};

/* struct definitions */
struct ALIGN(CACHE_LINE_SIZE) Task {
    TaskFunction*   function;
    void*           user_data;

    // pad the task to the average current cache line size (64 bytes) to avoid
    // false sharing
    char    _padding[CACHE_LINE_SIZE - (sizeof(void*) * 2)];
};

typedef struct Thread {
    Task                    tasks[kMaxTasks];
    TaskQueue<kMaxTasks>    queue;
    TaskPool*   pool;
    std::thread thread;
    int64_t     thread_id;
} Thread;

struct TaskPool {
    AllocationCallbacks allocator;
    std::atomic<int>    num_idle_threads;
    int                 num_threads;
    Thread              threads[1];
};


namespace {


/* thread-local thread id */
thread_local int64_t _thread_id;

/* static methods */
void* _DefaultAllocate(size_t size, void* user_data)
{
    (void)user_data;
    return malloc(size);
}
void _DefaultFree(void* data, void* user_data)
{
    (void)user_data;
    free(data);
}
AllocationCallbacks const kDefaultAllocator = {
    _DefaultAllocate,
    _DefaultFree,
    nullptr,
};

int _ThreadProc(void* data)
{
    Thread* thread = (Thread*)data;
    _thread_id = thread->thread_id;
    thread->pool->num_idle_threads.fetch_add(1, std::memory_order_relaxed);
    return 0;
}

} // anonymous namespace

/* public methods */
TaskPool* tpCreatePool(int num_threads, AllocationCallbacks const* allocator)
{
    if (allocator == nullptr) {
        allocator = &kDefaultAllocator;
    }

    num_threads = num_threads + 1; // add one for the main thread
    size_t const total_size = sizeof(TaskPool) + sizeof(Thread) * (num_threads - 1);
    TaskPool* pool = (TaskPool*)allocator->allocate_function(total_size, allocator->user_data);
    if (pool == nullptr) {
        return pool;
    }
    pool->allocator = *allocator;
    pool->num_threads = num_threads;
    pool->num_idle_threads = 0;

    memset(pool->threads, 0, sizeof(pool->threads[0])*num_threads);

    pool->threads[0].thread_id = _thread_id;
    pool->threads[0].pool = pool;
    for (int ii = 1; ii < pool->num_threads; ++ii) {
        pool->threads[ii].thread_id = ii;
        pool->threads[ii].pool = pool;
        pool->threads[ii].thread = std::thread(_ThreadProc,
                                               &pool->threads[ii]);
    }

    return pool;
}

void tpDestroyPool(TaskPool* pool)
{
    pool->allocator.free_function(pool, pool->allocator.user_data);
}

int tpNumThreads(TaskPool const* pool)
{
    if (pool == nullptr) {
        return 0;
    }
    return pool->num_threads;
}

int tpNumIdleThreads(TaskPool const* pool)
{
    if (pool == nullptr) {
        return 0;
    }
    return pool->num_idle_threads;
}

Task* tpSpawnTask(TaskPool* pool, TaskFunction* function, void* data)
{
    function(data);
    (void)pool;
    return (Task*)0xFF;
}

void tpWaitForTask(TaskPool* pool, Task* task)
{
    (void)pool;
    (void)task;
}

void tpFinishAllWork(TaskPool* pool)
{
    while (pool->num_idle_threads != pool->num_threads - 1)
        ;
}

