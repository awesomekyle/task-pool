#include <stdlib.h>
#include "task-pool/task-pool.h"

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

/* struct definitions */
struct TaskPool {
    AllocationCallbacks allocator;
    int num_threads;
};

/* public methods */
TaskPool* tpCreatePool(int num_threads, AllocationCallbacks const* allocator)
{
    AllocationCallbacks const default_allocator = {
        .allocate_function = _DefaultAllocate,
        .free_function = _DefaultFree,
        .user_data = NULL,
    };
    if (allocator == NULL) {
        allocator = &default_allocator;
    }

    TaskPool* pool = allocator->allocate_function(sizeof(*pool), allocator->user_data);
    if (pool == NULL) {
        return pool;
    }
    pool->allocator = *allocator;
    pool->num_threads = num_threads;
    return pool;
}

void tpDestroyPool(TaskPool* pool)
{
    pool->allocator.free_function(pool, pool->allocator.user_data);
}

