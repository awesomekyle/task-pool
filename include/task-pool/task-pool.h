#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct TaskPool TaskPool;
typedef void* (tpAllocator)(size_t, void*);

TaskPool* tpCreatePool(int num_threads, tpAllocator* allocator, void* user_data);
void tpDestroyPool(TaskPool* pool);


#ifdef __cplusplus
} // extern "C" {
#endif /* __cplusplus */
