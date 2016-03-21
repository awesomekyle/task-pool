#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:28182) // dereferencing NULL pointer (within Gtest)
    #include <gtest/gtest.h>
    #pragma warning(pop)
#else
    #include <gtest/gtest.h>
#endif // #if defined(_MSC_VER)

#include "../src/task-queue.h"

namespace {

TEST(TaskQueue, CreateQueue)
{
    TaskQueue queue = { {0} };
    ASSERT_EQ(0, spmcqSize(&queue));
}
TEST(TaskQueue, PushItem)
{
    TaskQueue queue = { {0} };
    int const result = spmcqPush(&queue, (struct Task*)0x1234);
    ASSERT_EQ(0, result);
    ASSERT_EQ(1, spmcqSize(&queue));
}
TEST(TaskQueue, PushItemFailsWhenQueueIsFull)
{
    TaskQueue queue = { {0} };
    // fill queue
    for (int ii = 0; ii < kMaxQueueSize - 1; ++ii) {
        spmcqPush(&queue, (struct Task*)0x1234);
    }
    int result = spmcqPush(&queue, (struct Task*)0x1234);
    ASSERT_EQ(0, result);
    ASSERT_EQ(kMaxQueueSize, spmcqSize(&queue));
    // try pushing one more
    result = spmcqPush(&queue, (struct Task*)0x1234);
    ASSERT_NE(0, result);
    ASSERT_EQ(kMaxQueueSize, spmcqSize(&queue));
}

TEST(TaskQueue, PopItemFromEmptyQueueReturnsNULL)
{
    TaskQueue queue = { {0} };
    ASSERT_EQ(nullptr, spmcqPop(&queue));
}
TEST(TaskQueue, PopValidItem)
{
    TaskQueue queue = { {0} };
    spmcqPush(&queue, (struct Task*)0x1234);
    struct Task* const value = spmcqPop(&queue);
    ASSERT_EQ((struct Task*)0x1234, value);
    ASSERT_EQ(0, spmcqSize(&queue));
}

TEST(TaskQueue, PopIsLIFOOrder)
{
    TaskQueue queue = { {0} };
    spmcqPush(&queue, (struct Task*)0x1);
    spmcqPush(&queue, (struct Task*)0x2);
    spmcqPush(&queue, (struct Task*)0x3);
    struct Task* value = spmcqPop(&queue);
    ASSERT_EQ((struct Task*)0x3, value);
    value = spmcqPop(&queue);
    ASSERT_EQ((struct Task*)0x2, value);
    value = spmcqPop(&queue);
    ASSERT_EQ((struct Task*)0x1, value);
}
TEST(TaskQueue, PopEmptiesQueue)
{
    TaskQueue queue = { {0} };
    spmcqPush(&queue, (struct Task*)0x1);
    spmcqPush(&queue, (struct Task*)0x2);
    spmcqPush(&queue, (struct Task*)0x3);
    spmcqPop(&queue);
    spmcqPop(&queue);
    spmcqPop(&queue);
    ASSERT_EQ(NULL, spmcqPop(&queue));
    ASSERT_EQ(0, spmcqSize(&queue));
}

TEST(TaskQueue, StealItemFromEmptyQueueReturnsNULL)
{
    TaskQueue queue = { {0} };
    ASSERT_EQ(nullptr, spmcqSteal(&queue));
}
TEST(TaskQueue, StealValidItemFromQueue)
{
    TaskQueue queue = { {0} };
    spmcqPush(&queue, (struct Task*)0x1234);
    ASSERT_EQ((struct Task*)0x1234, spmcqSteal(&queue));
}

TEST(TaskQueue, StealIsFIFOOrder)
{
    TaskQueue queue = { {0} };
    spmcqPush(&queue, (struct Task*)0x1);
    spmcqPush(&queue, (struct Task*)0x2);
    spmcqPush(&queue, (struct Task*)0x3);
    struct Task* value = spmcqSteal(&queue);
    ASSERT_EQ((struct Task*)0x1, value);
    value = spmcqSteal(&queue);
    ASSERT_EQ((struct Task*)0x2, value);
    value = spmcqSteal(&queue);
    ASSERT_EQ((struct Task*)0x3, value);
}
TEST(TaskQueue, StealEmptiesQueue)
{
    TaskQueue queue = { {0} };
    spmcqPush(&queue, (struct Task*)0x1);
    spmcqPush(&queue, (struct Task*)0x2);
    spmcqPush(&queue, (struct Task*)0x3);
    spmcqSteal(&queue);
    spmcqSteal(&queue);
    spmcqSteal(&queue);
    ASSERT_EQ(NULL, spmcqSteal(&queue));
    ASSERT_EQ(0, spmcqSize(&queue));
}

}
