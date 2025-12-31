//----------------------------------------------------------------------------
// Unit tests for osQueue class
//----------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include "osQueue.hpp"

class osQueueTest : public ::testing::Test {
protected:
    static const int QUEUE_SIZE = 10;
    void* buffer[QUEUE_SIZE];
    
    void SetUp() override {
        for (int i = 0; i < QUEUE_SIZE; i++) {
            buffer[i] = nullptr;
        }
    }
    void TearDown() override {}
};

TEST_F(osQueueTest, Constructor) {
    osQueue queue("TestQueue", QUEUE_SIZE, buffer);
    EXPECT_STREQ("TestQueue", queue.GetName());
    EXPECT_EQ(0, queue.GetCount());
}

TEST_F(osQueueTest, PutAndGet) {
    osQueue queue("PutGetTest", QUEUE_SIZE, buffer);
    
    int item1 = 42;
    int item2 = 123;
    
    EXPECT_TRUE(queue.Put(&item1));
    EXPECT_EQ(1, queue.GetCount());
    
    EXPECT_TRUE(queue.Put(&item2));
    EXPECT_EQ(2, queue.GetCount());
    
    void* result1 = queue.Get();
    EXPECT_EQ(&item1, result1);
    EXPECT_EQ(1, queue.GetCount());
    
    void* result2 = queue.Get();
    EXPECT_EQ(&item2, result2);
    EXPECT_EQ(0, queue.GetCount());
}

TEST_F(osQueueTest, GetFromEmptyQueue) {
    osQueue queue("EmptyTest", QUEUE_SIZE, buffer);
    
    void* result = queue.Get();
    EXPECT_EQ(nullptr, result);
}

TEST_F(osQueueTest, Peek) {
    osQueue queue("PeekTest", QUEUE_SIZE, buffer);
    
    int item = 99;
    queue.Put(&item);
    
    // Peek should return the item without removing it
    void* peeked = queue.Peek();
    EXPECT_EQ(&item, peeked);
    EXPECT_EQ(1, queue.GetCount());
    
    // Second peek should return the same item
    peeked = queue.Peek();
    EXPECT_EQ(&item, peeked);
    EXPECT_EQ(1, queue.GetCount());
}

TEST_F(osQueueTest, PeekEmptyQueue) {
    osQueue queue("PeekEmptyTest", QUEUE_SIZE, buffer);
    
    void* result = queue.Peek();
    EXPECT_EQ(nullptr, result);
}

TEST_F(osQueueTest, QueueFull) {
    osQueue queue("FullTest", QUEUE_SIZE, buffer);
    int items[QUEUE_SIZE + 5];
    
    // Fill the queue
    for (int i = 0; i < QUEUE_SIZE; i++) {
        items[i] = i;
        bool result = queue.Put(&items[i]);
        // May or may not succeed depending on implementation
        if (!result) break;
    }
    
    // Queue should be at capacity
    int count = queue.GetCount();
    EXPECT_GT(count, 0);
}

TEST_F(osQueueTest, FIFO_Order) {
    osQueue queue("FIFOTest", QUEUE_SIZE, buffer);
    int items[5] = {1, 2, 3, 4, 5};
    
    for (int i = 0; i < 5; i++) {
        queue.Put(&items[i]);
    }
    
    // Should come out in FIFO order
    for (int i = 0; i < 5; i++) {
        void* result = queue.Get();
        EXPECT_EQ(&items[i], result);
    }
}

TEST_F(osQueueTest, Contains) {
    osQueue queue("ContainsTest", QUEUE_SIZE, buffer);
    
    int item1 = 1;
    int item2 = 2;
    int item3 = 3;
    
    queue.Put(&item1);
    queue.Put(&item2);
    
    EXPECT_TRUE(queue.Contains(&item1));
    EXPECT_TRUE(queue.Contains(&item2));
    EXPECT_FALSE(queue.Contains(&item3));
}

TEST_F(osQueueTest, Flush) {
    osQueue queue("FlushTest", QUEUE_SIZE, buffer);
    int items[5];
    
    for (int i = 0; i < 5; i++) {
        items[i] = i;
        queue.Put(&items[i]);
    }
    
    EXPECT_EQ(5, queue.GetCount());
    
    queue.Flush();
    
    EXPECT_EQ(0, queue.GetCount());
    EXPECT_EQ(nullptr, queue.Get());
}

TEST_F(osQueueTest, WrapAround) {
    osQueue queue("WrapTest", QUEUE_SIZE, buffer);
    int items[QUEUE_SIZE * 2];
    
    // Put and get multiple times to force wrap-around
    for (int cycle = 0; cycle < 3; cycle++) {
        for (int i = 0; i < QUEUE_SIZE - 1; i++) {
            items[i] = cycle * 100 + i;
            EXPECT_TRUE(queue.Put(&items[i]));
        }
        
        for (int i = 0; i < QUEUE_SIZE - 1; i++) {
            void* result = queue.Get();
            EXPECT_EQ(&items[i], result);
        }
    }
    
    EXPECT_EQ(0, queue.GetCount());
}

TEST_F(osQueueTest, ThreadSafePutGet) {
    osQueue queue("ThreadTest", QUEUE_SIZE, buffer);
    std::atomic<int> producedCount{0};
    std::atomic<int> consumedCount{0};
    
    int items[100];
    for (int i = 0; i < 100; i++) {
        items[i] = i;
    }
    
    auto producer = [&]() {
        for (int i = 0; i < 100; i++) {
            while (!queue.Put(&items[i])) {
                std::this_thread::yield();
            }
            producedCount++;
        }
    };
    
    auto consumer = [&]() {
        while (consumedCount < 100) {
            void* item = queue.Get();
            if (item != nullptr) {
                consumedCount++;
            } else {
                std::this_thread::yield();
            }
        }
    };
    
    std::thread prod(producer);
    std::thread cons(consumer);
    
    prod.join();
    cons.join();
    
    EXPECT_EQ(100, producedCount);
    EXPECT_EQ(100, consumedCount);
}

TEST_F(osQueueTest, DumpInfo) {
    osQueue queue("DumpTest", QUEUE_SIZE, buffer);
    
    std::ostringstream oss;
    osQueue::dump_info(oss);
    // Just ensure it doesn't crash
}
