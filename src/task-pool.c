#include <stdlib.h>
#include <string.h>
#include "task-pool/task-pool.h"
#include "task-queue.h"

#if defined(_MSC_VER)
    #define ALIGN(x) __declspec(align(x))
#elif defined(__GNUC__)
    #define ALIGN(x) __attribute__ ((aligned(x)))
#endif

#if defined(_MSC_VER)
    #define thread_local __declspec(thread)
#elif defined(__GNUC__)
    #define thread_local __thread
#endif

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

/* thread-local thread id */
static thread_local int64_t _thread_id;

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
    int64_t     thread_id;
} Thread;

struct TaskPool {
    AllocationCallbacks allocator;
    int64_t num_threads;
    Thread  threads[1];
};

/* public methods */
TaskPool* tpCreatePool(int num_threads, AllocationCallbacks const* allocator)
{
    if (allocator == NULL) {
        allocator = &kDefaultAllocator;
    }

    size_t const total_size = sizeof(TaskPool) + sizeof(Thread) * (num_threads - 1);
    TaskPool* pool = allocator->allocate_function(total_size, allocator->user_data);
    if (pool == NULL) {
        return pool;
    }
    pool->allocator = *allocator;
    pool->num_threads = num_threads;

    memset(pool->threads, 0, sizeof(pool->threads[0])*num_threads);

    pool->threads[0].thread_id = _thread_id;
    pool->threads[0].pool = pool;

    return pool;
}

void tpDestroyPool(TaskPool* pool)
{
    pool->allocator.free_function(pool, pool->allocator.user_data);
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

