#include <stdlib.h>
#include <string.h>
#include "task-pool/task-pool.h"
#include "task-queue.h"

#if defined(_MSC_VER)
    #include <intrin.h>
    #define ALIGN(x) __declspec(align(x))
    #define thread_local __declspec(thread)
    #define AtomicAdd(val, add) _InterlockedExchangeAdd((volatile long*)val, add)
#elif defined(__GNUC__)
    #define ALIGN(x) __attribute__ ((aligned(x)))
    #define thread_local __thread
    #define AtomicAdd(val, add) __sync_add_and_fetch(val, add)
#endif

/* constants */
#define CACHE_LINE_SIZE 64
enum {
    kMaxTasks = 1024,
};

/* struct definitions */
typedef struct ALIGN(CACHE_LINE_SIZE) Task {
    TaskFunction*   function;
    void*           user_data;

    // pad the task to the average current cache line size (64 bytes) to avoid
    // false sharing
    char    _padding[CACHE_LINE_SIZE - (sizeof(void*) * 2)];
} Task;

typedef struct Thread {
    Task        tasks[kMaxTasks];
    TaskQueue   queue;
    TaskPool*   pool;
    thrd_t      thread;
    int64_t     thread_id;
} Thread;

struct TaskPool {
    AllocationCallbacks allocator;
    volatile int    num_idle_threads;
    int             num_threads;
    Thread          threads[1];
};

/* thread-local thread id */
static thread_local int64_t _thread_id;

/* static methods */
static void* _DefaultAllocate(size_t size, void* user_data)
{
    (void)user_data;
    return malloc(size);
}
static void _DefaultFree(void* data, void* user_data)
{
    (void)user_data;
    free(data);
}
static AllocationCallbacks const kDefaultAllocator = {
    .allocate_function = _DefaultAllocate,
    .free_function = _DefaultFree,
    .user_data = NULL,
};

static int _ThreadProc(void* data)
{
    Thread* thread = (Thread*)data;
    _thread_id = thread->thread_id;
    AtomicAdd(&thread->pool->num_idle_threads, 1);
    return 0;
}


/* public methods */
TaskPool* tpCreatePool(int num_threads, AllocationCallbacks const* allocator)
{
    if (allocator == NULL) {
        allocator = &kDefaultAllocator;
    }

    num_threads = num_threads + 1; // add one for the main thread
    size_t const total_size = sizeof(TaskPool) + sizeof(Thread) * (num_threads - 1);
    TaskPool* pool = allocator->allocate_function(total_size, allocator->user_data);
    if (pool == NULL) {
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
        int const thread_create_result = std_thrd_create(&pool->threads[ii].thread,
                                                         _ThreadProc,
                                                         &pool->threads[ii]);
        assert(thread_create_result == thrd_success);
    }

    return pool;
}

void tpDestroyPool(TaskPool* pool)
{
    pool->allocator.free_function(pool, pool->allocator.user_data);
}

int tpNumThreads(TaskPool const* pool)
{
    if (pool == NULL) {
        return 0;
    }
    return pool->num_threads;
}

int tpNumIdleThreads(TaskPool const* pool)
{
    if (pool == NULL) {
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

