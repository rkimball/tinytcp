#include <gtest/gtest.h>
#include "Utility.hpp"

TEST(UtilityTest, Unpack8Test) {
    // Test case 1: Unpacking a single byte
    uint8_t data1[] = {0xAB};
    uint8_t result1 = Unpack8(data1, 0);
    EXPECT_EQ(result1, 0xAB);

    // Test case 2: Unpacking a byte at a non-zero offset
    uint8_t data2[] = {0x12, 0x34, 0x56};
    uint8_t result2 = Unpack8(data2, 1);
    EXPECT_EQ(result2, 0x34);

    // Test case 3: Unpacking a byte from a larger array
    uint8_t data3[] = {0x11, 0x22, 0x33, 0x44, 0x55};
    uint8_t result3 = Unpack8(data3, 3);
    EXPECT_EQ(result3, 0x44);
}

TEST(UtilityTest, Unpack16Test) {
    // Test case 1: Unpacking two bytes from a single byte array
    uint8_t data1[] = {0xAB, 0xCD};
    uint16_t result1 = Unpack16(data1, 0);
    EXPECT_EQ(result1, 0xABCD);

    // Test case 2: Unpacking two bytes at a non-zero offset
    uint8_t data2[] = {0x12, 0x34, 0x56, 0x78};
    uint16_t result2 = Unpack16(data2, 1);
    EXPECT_EQ(result2, 0x3456);

    // Test case 3: Unpacking two bytes from a larger array
    uint8_t data3[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint16_t result3 = Unpack16(data3, 3);
    EXPECT_EQ(result3, 0x4455);
}

TEST(UtilityTest, Unpack32Test) {
    // Test case 1: Unpacking four bytes from a single byte array
    uint8_t data1[] = {0xAB, 0xCD, 0xEF, 0x12};
    uint32_t result1 = Unpack32(data1, 0);
    EXPECT_EQ(result1, 0xABCDEF12);

    // Test case 2: Unpacking four bytes at a non-zero offset
    uint8_t data2[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    uint32_t result2 = Unpack32(data2, 2);
    EXPECT_EQ(result2, 0x56789ABC);

    // Test case 3: Unpacking four bytes from a larger array
    uint8_t data3[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint32_t result3 = Unpack32(data3, 4);
    EXPECT_EQ(result3, 0x55667788);
}

TEST(UtilityTest, Pack8Test) {
    // Test case 1: Packing a single byte
    uint8_t data1[1];
    size_t offset1 = Pack8(data1, 0, 0xAB);
    EXPECT_EQ(offset1, 1);
    EXPECT_EQ(data1[0], 0xAB);

    // Test case 2: Packing a byte at a non-zero offset
    uint8_t data2[3] = {0x00, 0x00, 0x00};
    size_t offset2 = Pack8(data2, 1, 0x34);
    EXPECT_EQ(offset2, 2);
    EXPECT_EQ(data2[1], 0x34);

    // Test case 3: Packing a byte in a larger array
    uint8_t data3[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    size_t offset3 = Pack8(data3, 3, 0x44);
    EXPECT_EQ(offset3, 4);
    EXPECT_EQ(data3[3], 0x44);
}

TEST(UtilityTest, Pack16Test) {
    // Test case 1: Packing a single byte
    uint8_t data1[2];
    size_t offset1 = Pack16(data1, 0, 0xABCD);
    EXPECT_EQ(offset1, 2);
    EXPECT_EQ(data1[0], 0xAB);
    EXPECT_EQ(data1[1], 0xCD);

    // Test case 2: Packing a byte at a non-zero offset
    uint8_t data2[4] = {0x00, 0x00, 0x00, 0x00};
    size_t offset2 = Pack16(data2, 1, 0x3456);
    EXPECT_EQ(offset2, 3);
    EXPECT_EQ(data2[1], 0x34);
    EXPECT_EQ(data2[2], 0x56);

    // Test case 3: Packing a byte in a larger array
    uint8_t data3[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t offset3 = Pack16(data3, 3, 0x4455);
    EXPECT_EQ(offset3, 5);
    EXPECT_EQ(data3[3], 0x44);
    EXPECT_EQ(data3[4], 0x55);
}

TEST(UtilityTest, Pack32Test) {
    // Test case 1: Packing a single byte
    uint8_t data1[4];
    size_t offset1 = Pack32(data1, 0, 0xABCDEF12);
    EXPECT_EQ(offset1, 4);
    EXPECT_EQ(data1[0], 0xAB);
    EXPECT_EQ(data1[1], 0xCD);
    EXPECT_EQ(data1[2], 0xEF);
    EXPECT_EQ(data1[3], 0x12);

    // Test case 2: Packing a byte at a non-zero offset
    uint8_t data2[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t offset2 = Pack32(data2, 2, 0x56789ABC);
    EXPECT_EQ(offset2, 6);
    EXPECT_EQ(data2[0], 0);
    EXPECT_EQ(data2[1], 0);
    EXPECT_EQ(data2[2], 0x56);
    EXPECT_EQ(data2[3], 0x78);
    EXPECT_EQ(data2[4], 0x9A);
    EXPECT_EQ(data2[5], 0xBC);

    // Test case 3: Packing a byte in a larger array
    uint8_t data3[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t offset3 = Pack32(data3, 3, 0x55667788);
    EXPECT_EQ(offset3, 7);
    EXPECT_EQ(data3[0], 0);
    EXPECT_EQ(data3[1], 0);
    EXPECT_EQ(data3[2], 0);
    EXPECT_EQ(data3[3], 0x55);
    EXPECT_EQ(data3[4], 0x66);
    EXPECT_EQ(data3[5], 0x77);
    EXPECT_EQ(data3[6], 0x88);
    EXPECT_EQ(data3[7], 0);
}
