#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:28182) // dereferencing NULL pointer (within Gtest)
    #include <gtest/gtest.h>
    #pragma warning(pop)
#else
    #include <gtest/gtest.h>
#endif // #if defined(_MSC_VER)

#include "../src/spmc-queue.h"

namespace {

TEST(SPMCQueue, CreateQueue)
{
    SPMCQueue queue = { {0} };
    ASSERT_EQ(0, spmcqSize(&queue));
}
TEST(SPMCQueue, PushItem)
{
    SPMCQueue queue = { {0} };
    int const result = spmcqPush(&queue, (void*)0x1234);
    ASSERT_EQ(0, result);
    ASSERT_EQ(1, spmcqSize(&queue));
}
TEST(SPMCQueue, PushItemFailsWhenQueueIsFull)
{
    SPMCQueue queue = { {0} };
    // fill queue
    for (int ii = 0; ii < kMaxQueueSize - 1; ++ii) {
        spmcqPush(&queue, (void*)0x1234);
    }
    int result = spmcqPush(&queue, (void*)0x1234);
    ASSERT_EQ(0, result);
    ASSERT_EQ(kMaxQueueSize, spmcqSize(&queue));
    // try pushing one more
    result = spmcqPush(&queue, (void*)0x1234);
    ASSERT_NE(0, result);
    ASSERT_EQ(kMaxQueueSize, spmcqSize(&queue));
}

TEST(SPMCQueue, PopItemFromEmptyQueueReturnsNULL)
{
    SPMCQueue queue = { {0} };
    ASSERT_EQ(nullptr, spmcqPop(&queue));
}
TEST(SPMCQueue, PopValidItem)
{
    SPMCQueue queue = { {0} };
    spmcqPush(&queue, (void*)0x1234);
    void* const value = spmcqPop(&queue);
    ASSERT_EQ((void*)0x1234, value);
    ASSERT_EQ(0, spmcqSize(&queue));
}

TEST(SPMCQueue, PopIsLIFOOrder)
{
    SPMCQueue queue = { {0} };
    spmcqPush(&queue, (void*)0x1);
    spmcqPush(&queue, (void*)0x2);
    spmcqPush(&queue, (void*)0x3);
    void* value = spmcqPop(&queue);
    ASSERT_EQ((void*)0x3, value);
    value = spmcqPop(&queue);
    ASSERT_EQ((void*)0x2, value);
    value = spmcqPop(&queue);
    ASSERT_EQ((void*)0x1, value);
}
TEST(SPMCQueue, PopEmptiesQueue)
{
    SPMCQueue queue = { {0} };
    spmcqPush(&queue, (void*)0x1);
    spmcqPush(&queue, (void*)0x2);
    spmcqPush(&queue, (void*)0x3);
    spmcqPop(&queue);
    spmcqPop(&queue);
    spmcqPop(&queue);
    ASSERT_EQ(NULL, spmcqPop(&queue));
    ASSERT_EQ(0, spmcqSize(&queue));
}

TEST(SPMCQueue, StealItemFromEmptyQueueReturnsNULL)
{
    SPMCQueue queue = { {0} };
    ASSERT_EQ(nullptr, spmcqSteal(&queue));
}
TEST(SPMCQueue, StealValidItemFromQueue)
{
    SPMCQueue queue = { {0} };
    spmcqPush(&queue, (void*)0x1234);
    ASSERT_EQ((void*)0x1234, spmcqSteal(&queue));
}

TEST(SPMCQueue, StealIsFIFOOrder)
{
    SPMCQueue queue = { {0} };
    spmcqPush(&queue, (void*)0x1);
    spmcqPush(&queue, (void*)0x2);
    spmcqPush(&queue, (void*)0x3);
    void* value = spmcqSteal(&queue);
    ASSERT_EQ((void*)0x1, value);
    value = spmcqSteal(&queue);
    ASSERT_EQ((void*)0x2, value);
    value = spmcqSteal(&queue);
    ASSERT_EQ((void*)0x3, value);
}
TEST(SPMCQueue, StealEmptiesQueue)
{
    SPMCQueue queue = { {0} };
    spmcqPush(&queue, (void*)0x1);
    spmcqPush(&queue, (void*)0x2);
    spmcqPush(&queue, (void*)0x3);
    spmcqSteal(&queue);
    spmcqSteal(&queue);
    spmcqSteal(&queue);
    ASSERT_EQ(NULL, spmcqSteal(&queue));
    ASSERT_EQ(0, spmcqSize(&queue));
}

}
