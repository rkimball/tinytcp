//----------------------------------------------------------------------------
// Unit tests for osMutex class
//----------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include "osMutex.hpp"

class osMutexTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(osMutexTest, Constructor) {
    osMutex mutex("TestMutex");
    EXPECT_STREQ("TestMutex", mutex.GetName());
}

TEST_F(osMutexTest, ConstructorEmptyName) {
    osMutex mutex("");
    EXPECT_STREQ("", mutex.GetName());
}

TEST_F(osMutexTest, TakeAndGive) {
    osMutex mutex("TakeGiveTest");
    
    // Should be able to take and give without blocking
    mutex.Take(__FILE__, __LINE__);
    mutex.Give();
}

TEST_F(osMutexTest, MultipleTakeGiveCycles) {
    osMutex mutex("CycleTest");
    
    for (int i = 0; i < 10; i++) {
        mutex.Take(__FILE__, __LINE__);
        mutex.Give();
    }
}

TEST_F(osMutexTest, ProtectsSharedData) {
    osMutex mutex("SharedDataTest");
    int sharedCounter = 0;
    const int incrementsPerThread = 1000;
    const int numThreads = 4;
    
    auto incrementTask = [&]() {
        for (int i = 0; i < incrementsPerThread; i++) {
            mutex.Take(__FILE__, __LINE__);
            sharedCounter++;
            mutex.Give();
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(incrementTask);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Without proper mutex protection, this would likely fail
    EXPECT_EQ(incrementsPerThread * numThreads, sharedCounter);
}

TEST_F(osMutexTest, MutualExclusion) {
    osMutex mutex("ExclusionTest");
    std::atomic<int> insideCriticalSection{0};
    std::atomic<bool> exclusionViolated{false};
    
    auto task = [&]() {
        for (int i = 0; i < 100; i++) {
            mutex.Take(__FILE__, __LINE__);
            
            int count = ++insideCriticalSection;
            if (count != 1) {
                exclusionViolated = true;
            }
            
            // Small delay to increase chance of detecting violations
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            
            --insideCriticalSection;
            mutex.Give();
        }
    };
    
    std::thread t1(task);
    std::thread t2(task);
    
    t1.join();
    t2.join();
    
    EXPECT_FALSE(exclusionViolated);
}

TEST_F(osMutexTest, DumpInfo) {
    osMutex mutex("DumpTest");
    
    // Just ensure dump_info doesn't crash
    std::ostringstream oss;
    osMutex::dump_info(oss);
    // Output may or may not be empty depending on implementation
}
