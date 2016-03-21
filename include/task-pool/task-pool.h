#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct AllocationCallbacks {
    void* (*allocate_function)(size_t size, void* user_data);
    void (*free_function)(void* data, void* user_data);
    void* user_data;
} AllocationCallbacks;

typedef struct TaskPool TaskPool;
typedef struct Task Task;
typedef volatile int TaskCompletion;

typedef void (TaskFunction)(void* data);

/// @param [in] num_threads The number of additional threads to spawn. Set this
///     to the total number of hardware threads your machine has - 1 for the main
///     thread
TaskPool* tpCreatePool(int num_threads, AllocationCallbacks const* allocator);
void tpDestroyPool(TaskPool* pool);

int tpNumThreads(TaskPool const* pool);
int tpNumIdleThreads(TaskPool const* pool);

/// @param [in] function The function to call asynchronously
/// @param [in] data The data to pass to the function
/// @param [in,out] completion An integer that will be incremented by one when
///     the task starts and decremented upon completion of the task. This can be
///     used to tie tasks together by giving multiple tasks the same completion
void tpSpawnTask(TaskPool* pool, TaskFunction* function, void* data,
                 TaskCompletion* completion);

/// @brief This will wait until the specified completion is 0. The calling thread
///     will help process tasks while it's waiting.
/// @param [in] completion The compeltion event to wait for
void tpWaitForCompletion(TaskPool* pool, TaskCompletion* completion);

/// @brief This spins until all remaining work in the pool has completed. While
///     theres still work to be done, the caller thread helps complete it.
void tpFinishAllWork(TaskPool* pool);

#ifdef __cplusplus
} // extern "C" {
#endif /* __cplusplus */
