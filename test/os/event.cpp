#include <gtest/gtest.h>
#include "os/event.hpp"

TEST(EventTest, ConstructorTest) {
    // Test case 1: Check if the constructor initializes the name correctly
    const char* name1 = "Event1";
    os::Event event1(name1);
    EXPECT_STREQ(name1, event1.GetName());

    // Test case 2: Check if the constructor handles an empty name correctly
    const char* name2 = "";
    os::Event event2(name2);
    EXPECT_STREQ(name2, event2.GetName());

    // Test case 3: Check if the constructor handles a long name correctly
    const char* name3 = "ThisIsALongEventName";
    os::Event event3(name3);
    EXPECT_STREQ(name3, event3.GetName());
}
