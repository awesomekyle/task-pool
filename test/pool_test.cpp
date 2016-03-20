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
    TaskPool* pool = tpCreatePool(4, nullptr, nullptr);
    ASSERT_NE(nullptr, pool);
    tpDestroyPool(pool);
}

#if 0
TEST(TaskPool, SpawnTask)
{
    tp::Pool pool;

    auto callback = [](void*) {
    };

    auto* task = tp::SpawnTask(&pool, callback, nullptr);
    ASSERT_NE(nullptr, task);
}
#endif

}
