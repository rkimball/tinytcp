#include <stdint.h>
#include <stdio.h>
#include <string>
#include <time.h>

#include <gtest/gtest.h>

#include "os/time.hpp"

namespace
{

uint64_t MonotonicMicroseconds()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000 + static_cast<uint64_t>(ts.tv_nsec / 1000);
}

} // namespace

TEST(TimeTest, GetTimeIsTheMonotonicClockInMicroseconds)
{
    uint64_t before = MonotonicMicroseconds();
    uint64_t now = os::Time::GetTime();
    uint64_t after = MonotonicMicroseconds();
    EXPECT_GE(now, before);
    EXPECT_LE(now, after);
}

TEST(TimeTest, GetTimeNeverGoesBackwards)
{
    uint64_t previous = os::Time::GetTime();
    for (int i = 0; i < 10000; i++)
    {
        uint64_t now = os::Time::GetTime();
        ASSERT_GE(now, previous);
        previous = now;
    }
}

TEST(TimeTest, GetTimeAdvancesWhileSleeping)
{
    uint64_t start = os::Time::GetTime();
    timespec twenty_ms = {0, 20 * 1000 * 1000};
    nanosleep(&twenty_ms, nullptr);
    uint64_t elapsed = os::Time::GetTime() - start;
    EXPECT_GE(elapsed, 20000u);
    EXPECT_LT(elapsed, 5000000u);
}

TEST(TimeTest, GetTimestampHasHoursMinutesSecondsAndMilliseconds)
{
    std::string text = os::Time::GetTimestamp();
    unsigned hour, minute, second, millisecond;
    char extra;
    // %c only converts if something follows the milliseconds, so 4 means the text ends there
    ASSERT_EQ(4,
              sscanf(text.c_str(), "%u:%u:%u.%u%c", &hour, &minute, &second, &millisecond, &extra))
        << "unexpected format: " << text;
    EXPECT_LT(hour, 24u);
    EXPECT_LT(minute, 60u);
    EXPECT_LT(second, 60u);
    EXPECT_LT(millisecond, 1000u);
}

TEST(TimeTest, GetTimestampPadsMinutesSecondsAndMilliseconds)
{
    // h:mm:ss.mmm, so everything from the first colon on has a fixed width (:mm:ss.mmm).
    std::string text = os::Time::GetTimestamp();
    size_t colon = text.find(':');
    ASSERT_NE(std::string::npos, colon);
    EXPECT_EQ(10u, text.size() - colon) << text;
    EXPECT_EQ(':', text[colon + 3]);
    EXPECT_EQ('.', text[colon + 6]);
}

TEST(TimeTest, GetTimestampMatchesGetTime)
{
    const uint64_t day_ms = 24ull * 60 * 60 * 1000;

    uint64_t before = os::Time::GetTime() / 1000;
    std::string text = os::Time::GetTimestamp();
    uint64_t after = os::Time::GetTime() / 1000;

    unsigned hour, minute, second, millisecond;
    ASSERT_EQ(4, sscanf(text.c_str(), "%u:%u:%u.%u", &hour, &minute, &second, &millisecond));
    uint64_t stamp = ((hour * 60ull + minute) * 60 + second) * 1000 + millisecond;

    // The stamp was taken between the two GetTime() calls. Compare modulo one day because the
    // stamp only shows the time of day.
    uint64_t since_before = (stamp + day_ms - before % day_ms) % day_ms;
    EXPECT_LE(since_before, after - before + 1) << text;
}

TEST(TimeTest, GetTimestampAdvances)
{
    std::string first = os::Time::GetTimestamp();
    timespec fifty_ms = {0, 50 * 1000 * 1000};
    nanosleep(&fifty_ms, nullptr);
    std::string second = os::Time::GetTimestamp();
    EXPECT_NE(first, second);
}
