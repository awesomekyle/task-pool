#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:28182) // dereferencing NULL pointer (within Gtest)
    #include <gtest/gtest.h>
    #pragma warning(pop)
#else
    #include <gtest/gtest.h>
#endif // #if defined(_MSC_VER)
#include <atomic>

#include "task-pool/task-pool.h"

namespace {

TEST(TaskPool, CreatePool)
{
    TaskPool* pool = tpCreatePool(4, nullptr);
    ASSERT_NE(nullptr, pool);
    ASSERT_EQ(5, tpNumThreads(pool));
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

TEST(TaskPool, NullPoolHasNoThreads)
{
    TaskPool* pool = NULL;
    ASSERT_EQ(0, tpNumThreads(pool));
}

TEST(TaskPool, PoolIsIdleWhenAllWorkIsComplete)
{
    TaskPool* pool = tpCreatePool(4, nullptr);
    ASSERT_NE(nullptr, pool);
    tpFinishAllWork(pool);
    ASSERT_EQ(4, tpNumIdleThreads(pool));
    tpDestroyPool(pool);
}

struct TaskPoolTasks : public ::testing::Test {
    void SetUp(void)
    {
        pool = tpCreatePool(4, nullptr);
        ASSERT_NE(nullptr, pool);
    }
    void TearDown(void)
    {
        tpDestroyPool(pool);
    }

    TaskPool* pool = nullptr;
};

TEST_F(TaskPoolTasks, SpawnSimpleTask)
{
    auto const task_function = [](int, void* data) {
        (*(int*)data) = 123;
    };

    TaskCompletion completion = 0;
    int test_int = 0;
    tpSpawnTask(pool, task_function, &test_int, &completion);
    tpWaitForCompletion(pool, &completion);
    ASSERT_EQ(0, completion);
    ASSERT_EQ(123, test_int);
}
TEST_F(TaskPoolTasks, TasksRunOnOtherThreads)
{

    auto const task_function = [](int thread_id, void* data) {
        ((std::atomic<int>*)data)->fetch_add(thread_id);
    };

    TaskCompletion completion = 0;
    std::atomic<int> test_int = 0;
    for (int ii = 0; ii < 1024; ++ii) { // 1024 should be enough for another thread
        tpSpawnTask(pool, task_function, &test_int, &completion);
    }
    tpWaitForCompletion(pool, &completion);
    ASSERT_EQ(0, completion);
    ASSERT_GT(test_int, 0);
}


TEST_F(TaskPoolTasks, TaskStressTest)
{
    auto const task_function = [](int, void* data) {
        ((std::atomic<int>*)data)->fetch_add(1);
    };

    int const kTotalTasks = 1000 * 1000;
    TaskCompletion completion = 0;
    std::atomic<int> test_int = 0;
    for (int ii = 0; ii < kTotalTasks; ++ii) {
        tpSpawnTask(pool, task_function, &test_int, &completion);
    }
    tpWaitForCompletion(pool, &completion);
    ASSERT_EQ(0, completion);
    ASSERT_EQ(kTotalTasks, test_int.load());
}

}
