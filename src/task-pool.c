#include <stdlib.h>
#include "task-pool/task-pool.h"

/* static methods */
static void* _MallocAllocator(size_t size, void* user_data)
{
    (void)user_data;
    return malloc(size);
}

/* struct definitions */
struct TaskPool {
    int num_threads;
};

/* public methods */
TaskPool* tpCreatePool(int num_threads, tpAllocator* allocator, void* user_data)
{
    if (allocator == NULL) {
        allocator = _MallocAllocator;
        user_data = NULL;
    }

    TaskPool* pool = malloc(sizeof(*pool));
    if (pool == NULL) {
        return pool;
    }
    pool->num_threads = num_threads;
    return pool;
}

void tpDestroyPool(TaskPool* pool)
{
    free(pool);
}

