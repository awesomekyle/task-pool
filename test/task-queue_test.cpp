#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:28182) // dereferencing NULL pointer (within Gtest)
    #include <gtest/gtest.h>
    #pragma warning(pop)
#else
    #include <gtest/gtest.h>
#endif // #if defined(_MSC_VER)

#include "../src/task-queue.hpp"

namespace {

enum {
    kMaxQueueSize = 1024,
};

TEST(TaskQueue, CreateQueue)
{
    TaskQueue<kMaxQueueSize> queue;
    ASSERT_EQ(0, queue.size());
}
TEST(TaskQueue, PushItem)
{
    TaskQueue<kMaxQueueSize> queue;
    int const result = queue.push((struct Task*)0x1234);
    ASSERT_EQ(0, result);
    ASSERT_EQ(1, queue.size());
}
TEST(TaskQueue, PushItemFailsWhenQueueIsFull)
{
    TaskQueue<kMaxQueueSize> queue;
    // fill queue
    for (int ii = 0; ii < kMaxQueueSize - 1; ++ii) {
        queue.push((struct Task*)0x1234);
    }
    int result = queue.push((struct Task*)0x1234);
    ASSERT_EQ(0, result);
    ASSERT_EQ(kMaxQueueSize, queue.size());
    // try pushing one more
    result = queue.push((struct Task*)0x1234);
    ASSERT_NE(0, result);
    ASSERT_EQ(kMaxQueueSize, queue.size());
}

TEST(TaskQueue, PopItemFromEmptyQueueReturnsNULL)
{
    TaskQueue<kMaxQueueSize> queue;
    ASSERT_EQ(nullptr, queue.pop());
}
TEST(TaskQueue, PopValidItem)
{
    TaskQueue<kMaxQueueSize> queue;
    queue.push((struct Task*)0x1234);
    struct Task* const value = queue.pop();
    ASSERT_EQ((struct Task*)0x1234, value);
    ASSERT_EQ(0, queue.size());
}

TEST(TaskQueue, PopIsLIFOOrder)
{
    TaskQueue<kMaxQueueSize> queue;
    queue.push((struct Task*)0x1);
    queue.push((struct Task*)0x2);
    queue.push((struct Task*)0x3);
    struct Task* value = queue.pop();
    ASSERT_EQ((struct Task*)0x3, value);
    value = queue.pop();
    ASSERT_EQ((struct Task*)0x2, value);
    value = queue.pop();
    ASSERT_EQ((struct Task*)0x1, value);
}
TEST(TaskQueue, PopEmptiesQueue)
{
    TaskQueue<kMaxQueueSize> queue;
    queue.push((struct Task*)0x1);
    queue.push((struct Task*)0x2);
    queue.push((struct Task*)0x3);
    queue.pop();
    queue.pop();
    queue.pop();
    ASSERT_EQ(NULL, queue.pop());
    ASSERT_EQ(0, queue.size());
}

TEST(TaskQueue, StealItemFromEmptyQueueReturnsNULL)
{
    TaskQueue<kMaxQueueSize> queue;
    ASSERT_EQ(nullptr, queue.steal());
}
TEST(TaskQueue, StealValidItemFromQueue)
{
    TaskQueue<kMaxQueueSize> queue;
    queue.push((struct Task*)0x1234);
    ASSERT_EQ((struct Task*)0x1234, queue.steal());
}

TEST(TaskQueue, StealIsFIFOOrder)
{
    TaskQueue<kMaxQueueSize> queue;
    queue.push((struct Task*)0x1);
    queue.push((struct Task*)0x2);
    queue.push((struct Task*)0x3);
    struct Task* value = queue.steal();
    ASSERT_EQ((struct Task*)0x1, value);
    value = queue.steal();
    ASSERT_EQ((struct Task*)0x2, value);
    value = queue.steal();
    ASSERT_EQ((struct Task*)0x3, value);
}
TEST(TaskQueue, StealEmptiesQueue)
{
    TaskQueue<kMaxQueueSize> queue;
    queue.push((struct Task*)0x1);
    queue.push((struct Task*)0x2);
    queue.push((struct Task*)0x3);
    queue.steal();
    queue.steal();
    queue.steal();
    ASSERT_EQ(NULL, queue.steal());
    ASSERT_EQ(0, queue.size());
}

}
