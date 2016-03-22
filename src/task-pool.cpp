#include <stdlib.h>
#include <string.h>
#include <thread>
#include <condition_variable>
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
    // Xcode's Clang doesn't support thread_local for some reason, and
    // redefining the keyword is an error for clang
    #pragma clang diagnostic ignored "-Wkeyword-macro"
    #define thread_local __thread
#endif // defined(__MACH__)

/* constants */
#define CACHE_LINE_SIZE 64
enum {
    kMaxTasks = 1024,
    kTasksMask = kMaxTasks - 1,
};

/* struct definitions */
struct ALIGN(CACHE_LINE_SIZE) Task {
    TaskFunction*   function;
    void*           user_data;
    TaskCompletion* completion;

    // pad the task to the average current cache line size (64 bytes) to avoid
    // false sharing
    char    _padding[CACHE_LINE_SIZE - (sizeof(void*) * 3)];
};

struct Thread {
    Task                    tasks[kMaxTasks];
    TaskQueue<kMaxTasks>    queue;
    TaskPool*   pool;
    std::thread thread;
    uint64_t    num_tasks;
    int         thread_id;
};

struct TaskPool {
    AllocationCallbacks allocator;
    std::condition_variable wake_condition;
    std::mutex          wake_mutex;
    std::atomic<bool>   running;
    std::atomic<int>    num_idle_threads = {0};
    std::atomic<int>    in_progress_tasks = {0};
    int                 num_threads;
    Thread              threads[1];
};


namespace {


/* thread-local thread id */
thread_local int _thread_id;

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

Task* _GetTask(Thread* thread)
{
    TaskPool* pool = thread->pool;
    Task* task = thread->queue.pop();
    if (task == nullptr) {
        // round robin through threads
        for (int ii = 1; ii < pool->num_threads; ++ii) {
            int const other_thread_id = (_thread_id + ii) % pool->num_threads;
            assert(other_thread_id >= 0);
            assert(other_thread_id < pool->num_threads);
            auto& other_queue = pool->threads[other_thread_id].queue;

            task = other_queue.steal();
            if (task) {
                return task;
            }
        }
    }
    return task;
}

void _RunTask(TaskPool* pool, Task* task)
{
    task->function(_thread_id, task->user_data);
    pool->in_progress_tasks--;
    AtomicAdd(task->completion, -1);
    task->completion = nullptr;
}

Task* _AllocateTask(TaskPool* pool)
{
    Thread& thread = pool->threads[_thread_id];
    Task* task = nullptr;
    do {
        uint64_t const index = thread.num_tasks++;
        task = &thread.tasks[index & kTasksMask];
    } while (task->completion);
    return task;
}

void _ThreadProc(Thread* thread)
{
    assert(thread != nullptr);
    assert(thread->pool != nullptr);
    TaskPool* pool = thread->pool;
    _thread_id = thread->thread_id;
    do {
        // sleep
        {
            std::unique_lock<std::mutex> lock(pool->wake_mutex);
            if (pool->running.load() == false) {
                break;
            }
            pool->num_idle_threads++;
            pool->wake_condition.wait(lock);
            pool->num_idle_threads--;
        }
        if (pool->running.load() == false) {
            break;
        }
        Task* task = _GetTask(thread);
        while (task != nullptr) {
            _RunTask(pool, task);
            task = _GetTask(thread);
        }
    } while (pool->running.load());
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
    TaskPool* pool = new (allocator->allocate_function(total_size, allocator->user_data)) TaskPool;
    if (pool == nullptr) {
        return pool;
    }
    pool->allocator = *allocator;
    pool->num_threads = num_threads;
    pool->num_idle_threads = 0;
    pool->running.store(true);

    memset(pool->threads, 0, sizeof(pool->threads[0])*num_threads);
    std::atomic_thread_fence(std::memory_order_seq_cst);

    pool->threads[0].thread_id = _thread_id;
    pool->threads[0].pool = pool;
    for (int ii = 1; ii < pool->num_threads; ++ii) {
        pool->threads[ii].thread_id = ii;
        pool->threads[ii].pool = pool;
        assert(pool->threads[ii].pool);
        pool->threads[ii].thread = std::thread(_ThreadProc, &pool->threads[ii]);
    }
    while(pool->num_idle_threads.load() != num_threads-1)
        ; // wait for all threads to idle

    return pool;
}

void tpDestroyPool(TaskPool* pool)
{
    tpFinishAllWork(pool);
    {
        std::lock_guard<std::mutex> lock(pool->wake_mutex);
        pool->running.store(false);
        pool->wake_condition.notify_all();
    }
    for (int ii = 1; ii < pool->num_threads; ++ii) {
        pool->threads[ii].thread.join();
    }
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

void tpSpawnTask(TaskPool* pool, TaskFunction* function, void* data,
                 TaskCompletion* completion)
{
    AtomicAdd(completion, 1);
    pool->in_progress_tasks++;
    Task* task = _AllocateTask(pool);
    task->completion = completion;
    task->function = function;
    task->user_data = data;
    pool->threads[_thread_id].queue.push(task);
    pool->wake_condition.notify_all();
}

void tpWaitForCompletion(TaskPool* pool, TaskCompletion* completion)
{
    while (*completion) {
        Task* next_task = _GetTask(&pool->threads[_thread_id]);
        if (next_task) {
            _RunTask(pool, next_task);
        }
    }
}

void tpFinishAllWork(TaskPool* pool)
{
    Task* task = _GetTask(&pool->threads[_thread_id]);
    while (task || pool->in_progress_tasks.load() > 0) {
        if (task) {
            _RunTask(pool, task);
        }
        task = _GetTask(&pool->threads[_thread_id]);
    }
}

