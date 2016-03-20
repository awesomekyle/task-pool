#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct TaskPool TaskPool;

typedef struct AllocationCallbacks {
    void* (*allocate_function)(size_t size, void* user_data);
    void (*free_function)(void* data, void* user_data);
    void* user_data;
} AllocationCallbacks;

TaskPool* tpCreatePool(int num_threads, AllocationCallbacks const* allocator);
void tpDestroyPool(TaskPool* pool);


#ifdef __cplusplus
} // extern "C" {
#endif /* __cplusplus */
