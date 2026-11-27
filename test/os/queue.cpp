#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "helpers.hpp"
#include "os/queue.hpp"

using os_test::Contains;
using os_test::WaitUntil;

namespace
{

std::string DumpQueues()
{
    return os_test::Dump(os::Queue::dump_info);
}

// Non-null values to put in a queue: item(1), item(2) and so on.
void* item(uintptr_t number)
{
    return reinterpret_cast<void*>(number);
}

// A queue and the storage it needs.
class TestQueue
{
public:
    TestQueue(const char* name, int capacity)
        : storage(capacity > 0 ? capacity : 1)
        , queue(name, capacity, storage.data())
    {
    }

    os::Queue* operator->() { return &queue; }
    os::Queue& operator*() { return queue; }

    std::vector<void*> storage;
    os::Queue queue;
};

} // namespace

TEST(QueueTest, NameIsReturned)
{
    TestQueue q("name-test", 4);
    EXPECT_STREQ("name-test", q->GetName());
}

TEST(QueueTest, ANullNameIsTheEmptyString)
{
    TestQueue q(nullptr, 2);
    ASSERT_NE(nullptr, q->GetName());
    EXPECT_STREQ("", q->GetName());

    // It still works, and it is listed with an empty name, not with "(null)"
    EXPECT_TRUE(q->Put(item(1)));
    EXPECT_EQ(item(1), q->Get());
    std::string text = DumpQueues();
    EXPECT_FALSE(Contains(text, "(null)"));
    EXPECT_TRUE(Contains(text, "Queue  is size 2 and contains 0 objects\n"));
}

TEST(QueueTest, StartsEmpty)
{
    TestQueue q("empty", 4);
    EXPECT_EQ(0, q->GetCount());
    EXPECT_EQ(nullptr, q->Get());
    EXPECT_EQ(nullptr, q->Peek());
}

TEST(QueueTest, ItemsComeOutInTheOrderTheyWentIn)
{
    TestQueue q("fifo", 4);
    EXPECT_TRUE(q->Put(item(1)));
    EXPECT_TRUE(q->Put(item(2)));
    EXPECT_TRUE(q->Put(item(3)));
    EXPECT_EQ(3, q->GetCount());

    EXPECT_EQ(item(1), q->Get());
    EXPECT_EQ(item(2), q->Get());
    EXPECT_EQ(item(3), q->Get());
    EXPECT_EQ(0, q->GetCount());
    EXPECT_EQ(nullptr, q->Get());
}

TEST(QueueTest, PutFailsWhenTheQueueIsFull)
{
    TestQueue q("full", 3);
    EXPECT_TRUE(q->Put(item(1)));
    EXPECT_TRUE(q->Put(item(2)));
    EXPECT_TRUE(q->Put(item(3)));
    EXPECT_FALSE(q->Put(item(4)));
    EXPECT_EQ(3, q->GetCount());

    // The rejected item is not in the queue, and the others are untouched
    EXPECT_FALSE(q->Contains(item(4)));
    EXPECT_EQ(item(1), q->Get());
    EXPECT_EQ(item(2), q->Get());
    EXPECT_EQ(item(3), q->Get());
    EXPECT_EQ(nullptr, q->Get());
}

TEST(QueueTest, ARoomMakesPutWorkAgain)
{
    TestQueue q("room", 2);
    EXPECT_TRUE(q->Put(item(1)));
    EXPECT_TRUE(q->Put(item(2)));
    EXPECT_FALSE(q->Put(item(3)));
    EXPECT_EQ(item(1), q->Get());
    EXPECT_TRUE(q->Put(item(3)));
    EXPECT_EQ(item(2), q->Get());
    EXPECT_EQ(item(3), q->Get());
}

TEST(QueueTest, KeepsOrderWhenItWrapsAroundTheStorage)
{
    TestQueue q("wrap", 3);
    EXPECT_TRUE(q->Put(item(1)));
    EXPECT_TRUE(q->Put(item(2)));
    EXPECT_TRUE(q->Put(item(3)));
    EXPECT_EQ(item(1), q->Get());
    EXPECT_EQ(item(2), q->Get());
    EXPECT_TRUE(q->Put(item(4)));
    EXPECT_TRUE(q->Put(item(5)));
    EXPECT_FALSE(q->Put(item(6)));

    EXPECT_EQ(item(3), q->Get());
    EXPECT_EQ(item(4), q->Get());
    EXPECT_EQ(item(5), q->Get());
    EXPECT_EQ(nullptr, q->Get());
}

TEST(QueueTest, ManyTimesRoundTheStorage)
{
    TestQueue q("many", 5);
    uintptr_t next_in = 1;
    uintptr_t next_out = 1;
    for (int round = 0; round < 1000; round++)
    {
        // Fill it up to a different level each round, then empty it completely.
        int level = 1 + round % 5;
        for (int i = 0; i < level; i++)
        {
            ASSERT_TRUE(q->Put(item(next_in++)));
        }
        EXPECT_EQ(level, q->GetCount());
        for (int i = 0; i < level; i++)
        {
            ASSERT_EQ(item(next_out++), q->Get());
        }
        ASSERT_EQ(0, q->GetCount());
    }
}

TEST(QueueTest, PeekShowsTheNextItemWithoutRemovingIt)
{
    TestQueue q("peek", 4);
    q->Put(item(7));
    q->Put(item(8));
    EXPECT_EQ(item(7), q->Peek());
    EXPECT_EQ(item(7), q->Peek());
    EXPECT_EQ(2, q->GetCount());
    EXPECT_EQ(item(7), q->Get());
    EXPECT_EQ(item(8), q->Peek());
}

TEST(QueueTest, GetCountFollowsPutAndGet)
{
    TestQueue q("count", 4);
    EXPECT_EQ(0, q->GetCount());
    q->Put(item(1));
    EXPECT_EQ(1, q->GetCount());
    q->Put(item(2));
    EXPECT_EQ(2, q->GetCount());
    q->Get();
    EXPECT_EQ(1, q->GetCount());
    q->Get();
    EXPECT_EQ(0, q->GetCount());
    q->Get();
    EXPECT_EQ(0, q->GetCount());
}

TEST(QueueTest, ContainsFindsItemsThatAreInTheQueue)
{
    TestQueue q("contains", 4);
    q->Put(item(1));
    q->Put(item(2));
    q->Put(item(3));
    EXPECT_TRUE(q->Contains(item(1)));
    EXPECT_TRUE(q->Contains(item(2)));
    EXPECT_TRUE(q->Contains(item(3)));
    EXPECT_FALSE(q->Contains(item(4)));
}

TEST(QueueTest, ContainsIsFalseForItemsThatWereRemoved)
{
    TestQueue q("contains-removed", 4);
    q->Put(item(1));
    q->Put(item(2));
    q->Get();
    EXPECT_FALSE(q->Contains(item(1)));
    EXPECT_TRUE(q->Contains(item(2)));
}

TEST(QueueTest, ContainsWorksWhenTheQueueWrapsAround)
{
    TestQueue q("contains-wrap", 3);
    q->Put(item(1));
    q->Put(item(2));
    q->Put(item(3));
    q->Get();
    q->Get();
    q->Put(item(4));
    q->Put(item(5));
    // The items now run from the end of the storage back round to the start
    EXPECT_FALSE(q->Contains(item(1)));
    EXPECT_FALSE(q->Contains(item(2)));
    EXPECT_TRUE(q->Contains(item(3)));
    EXPECT_TRUE(q->Contains(item(4)));
    EXPECT_TRUE(q->Contains(item(5)));
}

TEST(QueueTest, ContainsOnAnEmptyQueueIsFalse)
{
    TestQueue q("contains-empty", 4);
    EXPECT_FALSE(q->Contains(item(1)));
}

TEST(QueueTest, FlushRemovesEveryItem)
{
    TestQueue q("flush", 4);
    q->Put(item(1));
    q->Put(item(2));
    q->Put(item(3));
    q->Flush();
    EXPECT_EQ(0, q->GetCount());
    EXPECT_EQ(nullptr, q->Get());
    EXPECT_FALSE(q->Contains(item(2)));

    // And it still works afterwards
    EXPECT_TRUE(q->Put(item(9)));
    EXPECT_EQ(item(9), q->Get());
}

TEST(QueueTest, FlushOnAnEmptyQueueDoesNothing)
{
    TestQueue q("flush-empty", 4);
    q->Flush();
    EXPECT_EQ(0, q->GetCount());
}

TEST(QueueTest, AQueueOfOne)
{
    TestQueue q("one", 1);
    EXPECT_TRUE(q->Put(item(1)));
    EXPECT_FALSE(q->Put(item(2)));
    EXPECT_EQ(item(1), q->Peek());
    EXPECT_EQ(item(1), q->Get());
    EXPECT_TRUE(q->Put(item(3)));
    EXPECT_EQ(item(3), q->Get());
}

TEST(QueueTest, AQueueOfZeroAcceptsNothing)
{
    os::Queue queue("zero", 0, nullptr);
    EXPECT_FALSE(queue.Put(item(1)));
    EXPECT_EQ(0, queue.GetCount());
    EXPECT_EQ(nullptr, queue.Get());
    EXPECT_EQ(nullptr, queue.Peek());
    EXPECT_FALSE(queue.Contains(item(1)));
}

TEST(QueueTest, AProducerAndAConsumerOnDifferentThreads)
{
    TestQueue q("producer-consumer", 8);
    const uintptr_t total = 20000;

    std::thread producer(
        [&]
        {
            for (uintptr_t i = 1; i <= total; i++)
            {
                while (!q->Put(item(i)))
                {
                    std::this_thread::yield();
                }
            }
        });

    uintptr_t expected = 1;
    bool in_order = true;
    while (expected <= total)
    {
        void* got = q->Get();
        if (got == nullptr)
        {
            std::this_thread::yield();
            continue;
        }
        if (got != item(expected))
        {
            in_order = false;
        }
        expected++;
    }
    producer.join();
    EXPECT_TRUE(in_order);
    EXPECT_EQ(0, q->GetCount());
}

TEST(QueueTest, SeveralProducersLoseNothing)
{
    TestQueue q("many-producers", 16);
    const int producers = 4;
    const uintptr_t each = 5000;

    std::vector<std::thread> threads;
    for (int p = 0; p < producers; p++)
    {
        threads.emplace_back(
            [&, p]
            {
                for (uintptr_t i = 0; i < each; i++)
                {
                    // Every value is unique: producer number in the high part, count in the low
                    // part
                    void* value = item((static_cast<uintptr_t>(p + 1) << 32) | (i + 1));
                    while (!q->Put(value))
                    {
                        std::this_thread::yield();
                    }
                }
            });
    }

    std::vector<uintptr_t> last(producers + 1, 0);
    uintptr_t received = 0;
    bool each_producer_in_order = true;
    while (received < producers * each)
    {
        void* got = q->Get();
        if (got == nullptr)
        {
            std::this_thread::yield();
            continue;
        }
        uintptr_t value = reinterpret_cast<uintptr_t>(got);
        uintptr_t producer = value >> 32;
        uintptr_t count = value & 0xffffffff;
        if (count != last[producer] + 1)
        {
            each_producer_in_order = false;
        }
        last[producer] = count;
        received++;
    }
    for (auto& thread : threads)
    {
        thread.join();
    }
    EXPECT_TRUE(each_producer_in_order);
    EXPECT_EQ(0, q->GetCount());
}

TEST(QueueTest, DumpInfoDescribesTheQueue)
{
    TestQueue q("described-queue", 6);
    q->Put(item(1));
    q->Put(item(2));
    EXPECT_TRUE(Contains(DumpQueues(), "Queue described-queue is size 6 and contains 2 objects\n"));

    q->Get();
    EXPECT_TRUE(Contains(DumpQueues(), "Queue described-queue is size 6 and contains 1 objects\n"));
}

TEST(QueueTest, DumpInfoListsEveryQueue)
{
    TestQueue first("list-all-first", 2);
    TestQueue second("list-all-second", 3);
    std::string text = DumpQueues();
    EXPECT_TRUE(Contains(text, "Queue list-all-first is size 2"));
    EXPECT_TRUE(Contains(text, "Queue list-all-second is size 3"));
}

TEST(QueueTest, DestroyedQueueIsRemovedFromDumpInfo)
{
    {
        TestQueue q("short-lived-queue", 2);
        EXPECT_TRUE(Contains(DumpQueues(), "short-lived-queue"));
    }
    EXPECT_FALSE(Contains(DumpQueues(), "short-lived-queue"));
}

TEST(QueueTest, DestroyedQueueSlotCanBeReused)
{
    for (int i = 0; i < 100; i++)
    {
        TestQueue q("recycled-queue", 2);
        q->Put(item(1));
        q->Get();
    }
    TestQueue last("recycled-last", 2);
    EXPECT_TRUE(Contains(DumpQueues(), "recycled-last"));
    EXPECT_FALSE(Contains(DumpQueues(), "recycled-queue"));
}

TEST(QueueTest, MoreQueuesThanTheListHoldsAllStillWork)
{
    // The list that dump_info() uses has room for 20 queues. The rest are not listed but work.
    const int count = 40;
    std::vector<std::string> names;
    for (int i = 0; i < count; i++)
    {
        names.push_back("overflow-queue-" + std::to_string(i));
    }
    {
        std::vector<std::unique_ptr<TestQueue>> queues;
        for (int i = 0; i < count; i++)
        {
            queues.emplace_back(new TestQueue(names[i].c_str(), 2));
        }
        for (int i = 0; i < count; i++)
        {
            EXPECT_TRUE((*queues[i])->Put(item(i + 1)));
            EXPECT_EQ(item(i + 1), (*queues[i])->Get());
        }

        std::string text = DumpQueues();
        int listed = 0;
        for (const std::string& name : names)
        {
            if (Contains(text, name))
            {
                listed++;
            }
        }
        EXPECT_GT(listed, 0);
        EXPECT_LT(listed, count);
    }

    std::string text = DumpQueues();
    for (const std::string& name : names)
    {
        EXPECT_FALSE(Contains(text, name)) << name << " is still listed after it was destroyed";
    }
}

// KNOWN BUG: Get() returns nullptr for an empty queue, and Flush() keeps calling Get() until it
// sees nullptr, so Flush() stops at the first nullptr that was put into the queue and leaves the
// items behind it. Enable this test when that is fixed.
TEST(QueueTest, DISABLED_FlushRemovesEverythingEvenWhenAnItemIsNull)
{
    TestQueue q("flush-null", 4);
    q->Put(nullptr);
    q->Put(item(1));
    q->Flush();
    EXPECT_EQ(0, q->GetCount());
}
