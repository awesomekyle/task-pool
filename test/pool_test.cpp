#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:28182) // dereferencing NULL pointer (within Gtest)
    #include <gtest/gtest.h>
    #pragma warning(pop)
#else
    #include <gtest/gtest.h>
#endif // #if defined(_MSC_VER)

#include "task-pool/task-pool.h"

namespace {

TEST(TaskPool, CreatePool)
{
    TaskPool* pool = tpCreatePool(4, nullptr);
    ASSERT_NE(nullptr, pool);
    tpDestroyPool(pool);
}
TEST(TaskPool, CreatePoolCustomAllocator)
{

    auto const allocate = [](size_t size, void* user_data) -> void* {
        (*(int*)user_data) = 123;
        return malloc(size);
    };
    auto const deallocate = [](void* data, void* user_data) -> void {
        (*(int*)user_data) = 456;
        free(data);
    };

    int test_int = 0;
    AllocationCallbacks const allocator = {
        allocate,
        deallocate,
        &test_int
    };
    TaskPool* pool = tpCreatePool(4, &allocator);
    ASSERT_NE(nullptr, pool);
    ASSERT_EQ(123, test_int);
    tpDestroyPool(pool);
    ASSERT_EQ(456, test_int);
}

}
