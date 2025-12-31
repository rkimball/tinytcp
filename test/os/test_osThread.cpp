//----------------------------------------------------------------------------
// Unit tests for osThread class
//----------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include "osThread.hpp"

class osThreadTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Simple thread entry function
static std::atomic<bool> threadExecuted{false};
static std::atomic<int> threadParam{0};

void SimpleThreadEntry(void* param) {
    threadExecuted = true;
    if (param != nullptr) {
        threadParam = *static_cast<int*>(param);
    }
}

void IncrementThreadEntry(void* param) {
    int* counter = static_cast<int*>(param);
    if (counter) {
        (*counter)++;
    }
}

void SleepThreadEntry(void* param) {
    // Just sleep briefly
    osThread::Sleep(50, __FILE__, __LINE__);
    threadExecuted = true;
}

TEST_F(osThreadTest, DefaultConstructor) {
    osThread thread;
    // Should be able to construct without crashing
    EXPECT_NE(nullptr, &thread);
}

TEST_F(osThreadTest, CreateAndRun) {
    threadExecuted = false;
    osThread thread;
    
    thread.Create(SimpleThreadEntry, "TestThread", 4096, 0, nullptr);
    
    // Give thread time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(threadExecuted);
}

TEST_F(osThreadTest, CreateWithParameter) {
    threadExecuted = false;
    threadParam = 0;
    int param = 42;
    osThread thread;
    
    thread.Create(SimpleThreadEntry, "ParamThread", 4096, 0, &param);
    
    // Give thread time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(threadExecuted);
    EXPECT_EQ(42, threadParam);
}

TEST_F(osThreadTest, GetName) {
    threadExecuted = false;
    osThread thread;
    thread.Create(SleepThreadEntry, "NamedThread", 4096, 0, nullptr);
    
    EXPECT_STREQ("NamedThread", thread.GetName());
    
    // Wait for thread to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
}

TEST_F(osThreadTest, MultipleThreads) {
    const int numThreads = 4;
    osThread threads[numThreads];
    int counters[numThreads] = {0};
    
    for (int i = 0; i < numThreads; i++) {
        char name[32];
        snprintf(name, sizeof(name), "Thread%d", i);
        threads[i].Create(IncrementThreadEntry, name, 4096, 0, &counters[i]);
    }
    
    // Give threads time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    for (int i = 0; i < numThreads; i++) {
        EXPECT_EQ(1, counters[i]);
    }
}

TEST_F(osThreadTest, Sleep) {
    auto start = std::chrono::steady_clock::now();
    
    osThread::Sleep(50, __FILE__, __LINE__);
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should have slept approximately 50ms (allow tolerance)
    EXPECT_GE(elapsed, 40);
    EXPECT_LE(elapsed, 150);
}

TEST_F(osThreadTest, USleep) {
    auto start = std::chrono::steady_clock::now();
    
    osThread::USleep(50000, __FILE__, __LINE__);  // 50000 microseconds = 50ms
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should have slept approximately 50ms (allow tolerance)
    EXPECT_GE(elapsed, 40);
    EXPECT_LE(elapsed, 150);
}

TEST_F(osThreadTest, GetCurrentThread) {
    osThread* current = osThread::GetCurrent();
    // May return nullptr if not in a managed thread
    // Just ensure it doesn't crash
    (void)current;
}

TEST_F(osThreadTest, DumpInfo) {
    osThread thread;
    
    std::ostringstream oss;
    osThread::dump_info(oss);
    // Just ensure it doesn't crash
}

TEST_F(osThreadTest, WaitForExit) {
    threadExecuted = false;
    osThread thread;
    
    thread.Create(SleepThreadEntry, "WaitThread", 4096, 0, nullptr);
    
    // Wait for thread to complete (give it enough time)
    thread.WaitForExit(500);
    
    // Give a bit more time for threadExecuted to be set
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(threadExecuted);
}
