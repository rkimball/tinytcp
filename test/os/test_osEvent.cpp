//----------------------------------------------------------------------------
// Unit tests for osEvent class
//----------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "osEvent.hpp"

class osEventTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(osEventTest, Constructor) {
    osEvent event("TestEvent");
    EXPECT_STREQ("TestEvent", event.GetName());
}

TEST_F(osEventTest, ConstructorEmptyName) {
    osEvent event("");
    EXPECT_STREQ("", event.GetName());
}

TEST_F(osEventTest, ConstructorLongName) {
    const char* longName = "ThisIsAVeryLongEventNameForTesting";
    osEvent event(longName);
    // Name might be truncated, just ensure it starts correctly
    EXPECT_EQ(0, strncmp(longName, event.GetName(), strlen(event.GetName())));
}

TEST_F(osEventTest, NotifyBeforeWait) {
    osEvent event("NotifyFirst");
    
    // Notify before wait - should not block
    event.Notify();
    
    // Wait with timeout - should return immediately since already notified
    bool result = event.Wait(__FILE__, __LINE__, 100);
    EXPECT_TRUE(result);
}

TEST_F(osEventTest, WaitTimeout) {
    osEvent event("TimeoutTest");
    
    auto start = std::chrono::steady_clock::now();
    bool result = event.Wait(__FILE__, __LINE__, 50);
    auto end = std::chrono::steady_clock::now();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should timeout and return false
    EXPECT_FALSE(result);
    // Should have waited approximately 50ms (allow some tolerance)
    EXPECT_GE(elapsed, 40);
    EXPECT_LE(elapsed, 200);
}

TEST_F(osEventTest, NotifyFromAnotherThread) {
    osEvent event("ThreadTest");
    std::atomic<bool> waitCompleted{false};
    std::atomic<bool> waitResult{false};
    
    std::thread waiter([&]() {
        bool result = event.Wait(__FILE__, __LINE__, 1000);
        waitResult = result;
        waitCompleted = true;
    });
    
    // Give the waiter thread time to start waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Notify from main thread
    event.Notify();
    
    waiter.join();
    EXPECT_TRUE(waitCompleted);
    EXPECT_TRUE(waitResult);
}

TEST_F(osEventTest, MultipleNotifies) {
    osEvent event("MultiNotify");
    
    // Multiple notifies before wait
    event.Notify();
    event.Notify();
    event.Notify();
    
    // First wait should succeed
    bool result1 = event.Wait(__FILE__, __LINE__, 100);
    EXPECT_TRUE(result1);
}

TEST_F(osEventTest, WaitIndefinitely) {
    osEvent event("IndefiniteWait");
    std::atomic<bool> waitCompleted{false};
    
    std::thread waiter([&]() {
        // Wait with -1 means wait forever
        bool result = event.Wait(__FILE__, __LINE__, -1);
        EXPECT_TRUE(result);
        waitCompleted = true;
    });
    
    // Give the waiter thread time to start waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Notify from main thread
    event.Notify();
    
    waiter.join();
    EXPECT_TRUE(waitCompleted);
}

TEST_F(osEventTest, DumpInfo) {
    osEvent event("DumpTest");
    
    // Just ensure dump_info doesn't crash
    std::ostringstream oss;
    osEvent::dump_info(oss);
    // Output should contain something
    EXPECT_FALSE(oss.str().empty());
}
