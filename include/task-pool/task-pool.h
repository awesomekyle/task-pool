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

typedef void (TaskFunction)(void* data);

TaskPool* tpCreatePool(int num_threads, AllocationCallbacks const* allocator);
void tpDestroyPool(TaskPool* pool);

/// @param [in] function The function to call asynchronously
/// @param [in] data The data to pass to the function
/// @return A handle to the newly spawned task. This can be queried later
Task* tpSpawnTask(TaskPool* pool, TaskFunction* function, void* data);

/// @brief This will wait until the specified task is complete. The calling thread
///     will help process tasks while it's waiting.
/// @param [in] task The task to wait for
void tpWaitForTask(TaskPool* pool, Task* task);

#ifdef __cplusplus
} // extern "C" {
#endif /* __cplusplus */
