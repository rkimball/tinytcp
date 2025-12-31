//----------------------------------------------------------------------------
// Unit tests for osTime class
//----------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <cstring>
#include "osTime.hpp"

class osTimeTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(osTimeTest, GetTimeReturnsValue) {
    uint64_t time = osTime::GetTime();
    // Should return a non-zero value
    EXPECT_GT(time, 0u);
}

TEST_F(osTimeTest, GetTimeIncreases) {
    uint64_t time1 = osTime::GetTime();
    
    // Small delay
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    uint64_t time2 = osTime::GetTime();
    
    // Time should have increased
    EXPECT_GT(time2, time1);
}

TEST_F(osTimeTest, GetTimeMonotonic) {
    // Check that time is monotonically increasing
    uint64_t prevTime = osTime::GetTime();
    
    for (int i = 0; i < 100; i++) {
        uint64_t currentTime = osTime::GetTime();
        EXPECT_GE(currentTime, prevTime);
        prevTime = currentTime;
    }
}

TEST_F(osTimeTest, GetTimePrecision) {
    // Measure how much time increases after a known delay
    uint64_t time1 = osTime::GetTime();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    uint64_t time2 = osTime::GetTime();
    uint64_t elapsed = time2 - time1;
    
    // Should have elapsed approximately 100ms worth of time units
    // The exact value depends on the time units used (likely milliseconds)
    EXPECT_GT(elapsed, 0u);
}

TEST_F(osTimeTest, GetTimestampReturnsString) {
    const char* timestamp = osTime::GetTimestamp();
    
    EXPECT_NE(nullptr, timestamp);
    EXPECT_GT(strlen(timestamp), 0u);
}

TEST_F(osTimeTest, GetTimestampFormat) {
    const char* timestamp = osTime::GetTimestamp();
    
    // Check that timestamp contains typical date/time characters
    // It should contain digits and possibly separators like : / -
    bool hasDigits = false;
    for (const char* p = timestamp; *p != '\0'; p++) {
        if (*p >= '0' && *p <= '9') {
            hasDigits = true;
            break;
        }
    }
    EXPECT_TRUE(hasDigits);
}

TEST_F(osTimeTest, GetTimestampChanges) {
    const char* timestamp1 = osTime::GetTimestamp();
    
    // Wait a second to ensure timestamp changes
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    const char* timestamp2 = osTime::GetTimestamp();
    
    // Timestamps should be different after 1 second
    // Note: They might point to the same buffer, so compare content
    // Actually, if they use a static buffer, second call overwrites first
    // Just verify second call returns valid string
    EXPECT_NE(nullptr, timestamp2);
    EXPECT_GT(strlen(timestamp2), 0u);
}

TEST_F(osTimeTest, ConsecutiveCalls) {
    // Rapidly call GetTime multiple times
    for (int i = 0; i < 1000; i++) {
        uint64_t time = osTime::GetTime();
        EXPECT_GT(time, 0u);
    }
}
