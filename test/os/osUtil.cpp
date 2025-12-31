//----------------------------------------------------------------------------
// Unit tests for osUtil (utility functions)
//----------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <sstream>
#include "osUtil.hpp"

class osUtilTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Tests for to_hex template function
// Note: to_hex with uint8_t needs cast to int for proper hex formatting
TEST_F(osUtilTest, ToHexUint8) {
    // uint8_t is treated as char by stringstream, so cast to int
    uint8_t value = 0xAB;
    std::string result = to_hex(static_cast<int>(value), 2);
    EXPECT_EQ("ab", result);
}

TEST_F(osUtilTest, ToHexUint8Zero) {
    uint8_t value = 0;
    std::string result = to_hex(static_cast<int>(value), 2);
    EXPECT_EQ("00", result);
}

TEST_F(osUtilTest, ToHexUint8Max) {
    uint8_t value = 0xFF;
    std::string result = to_hex(static_cast<int>(value), 2);
    EXPECT_EQ("ff", result);
}

TEST_F(osUtilTest, ToHexUint16) {
    uint16_t value = 0x1234;
    std::string result = to_hex(value);
    EXPECT_EQ("1234", result);
}

TEST_F(osUtilTest, ToHexUint16LeadingZeros) {
    uint16_t value = 0x00AB;
    std::string result = to_hex(value);
    EXPECT_EQ("00ab", result);
}

TEST_F(osUtilTest, ToHexUint32) {
    uint32_t value = 0xDEADBEEF;
    std::string result = to_hex(value);
    EXPECT_EQ("deadbeef", result);
}

TEST_F(osUtilTest, ToHexWithCustomWidth) {
    int value = 0x5;
    std::string result = to_hex(value, 4);
    EXPECT_EQ("0005", result);
}

TEST_F(osUtilTest, ToHexWithSmallerWidth) {
    uint16_t value = 0x1234;
    std::string result = to_hex(value, 2);
    // With smaller width, should still show the value
    EXPECT_EQ("1234", result);
}

// Tests for to_dec template function
TEST_F(osUtilTest, ToDecSingleDigit) {
    int value = 5;
    std::string result = to_dec(value, 1);
    EXPECT_EQ("5", result);
}

TEST_F(osUtilTest, ToDecWithPadding) {
    int value = 5;
    std::string result = to_dec(value, 3);
    EXPECT_EQ("005", result);
}

TEST_F(osUtilTest, ToDecZero) {
    int value = 0;
    std::string result = to_dec(value, 2);
    EXPECT_EQ("00", result);
}

TEST_F(osUtilTest, ToDecLargeNumber) {
    int value = 12345;
    std::string result = to_dec(value, 8);
    EXPECT_EQ("00012345", result);
}

TEST_F(osUtilTest, ToDecExactWidth) {
    int value = 123;
    std::string result = to_dec(value, 3);
    EXPECT_EQ("123", result);
}

TEST_F(osUtilTest, ToDecNegativeNumber) {
    int value = -5;
    std::string result = to_dec(value, 3);
    // Behavior with negative numbers depends on implementation
    // Just verify it doesn't crash and returns something
    EXPECT_FALSE(result.empty());
}

// Test combinations
TEST_F(osUtilTest, FormatMACAddressStyle) {
    uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    std::stringstream ss;
    for (int i = 0; i < 6; i++) {
        if (i > 0) ss << ":";
        ss << to_hex(static_cast<int>(mac[i]), 2);
    }
    EXPECT_EQ("00:11:22:33:44:55", ss.str());
}

TEST_F(osUtilTest, FormatIPAddressStyle) {
    uint8_t ip[4] = {192, 168, 1, 100};
    std::stringstream ss;
    for (int i = 0; i < 4; i++) {
        if (i > 0) ss << ".";
        ss << to_dec((int)ip[i], 1);
    }
    EXPECT_EQ("192.168.1.100", ss.str());
}

TEST_F(osUtilTest, FormatTimestampStyle) {
    int hours = 9;
    int minutes = 5;
    int seconds = 3;
    
    std::stringstream ss;
    ss << to_dec(hours, 2) << ":" << to_dec(minutes, 2) << ":" << to_dec(seconds, 2);
    EXPECT_EQ("09:05:03", ss.str());
}
