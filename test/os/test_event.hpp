#include <gtest/gtest.h>
#include "os/event.hpp"

TEST(osEventTest, ConstructorTest) {
    // Test case 1: Check if the constructor initializes the name correctly
    const char* name1 = "Event1";
    osEvent event1(name1);
    EXPECT_STREQ(name1, event1.getName());

    // Test case 2: Check if the constructor handles an empty name correctly
    const char* name2 = "";
    osEvent event2(name2);
    EXPECT_STREQ(name2, event2.getName());

    // Test case 3: Check if the constructor handles a long name correctly
    const char* name3 = "ThisIsALongEventName";
    osEvent event3(name3);
    EXPECT_STREQ(name3, event3.getName());
}