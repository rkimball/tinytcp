// Tests of the Internet checksum against RFC 1071 (Computing the Internet Checksum).
//
// The checksum is the one's complement of the one's complement sum of the data taken as 16 bit
// words. Names are Rfc1071_Section<n>_<requirement>.
#include <random>
#include <stdint.h>
#include <vector>

#include <gtest/gtest.h>

#include "tinytcp/FCS.hpp"

namespace tt = tinytcp;

namespace
{

// The algorithm of RFC 1071, written the plain way: 16 bit words, an odd last octet padded on the
// right with a zero octet, and the carries added back until none are left.
uint16_t ReferenceChecksum(const std::vector<uint8_t>& data)
{
    uint64_t sum = 0;
    for (size_t i = 0; i + 1 < data.size(); i += 2)
    {
        sum += (data[i] << 8) | data[i + 1];
    }
    if (data.size() % 2 != 0)
    {
        sum += data.back() << 8;
    }
    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return static_cast<uint16_t>(~sum);
}

std::vector<uint8_t> RandomData(std::mt19937& random, size_t length)
{
    std::vector<uint8_t> data(length);
    for (auto& byte : data)
    {
        byte = static_cast<uint8_t>(random());
    }
    return data;
}

uint16_t Checksum(const std::vector<uint8_t>& data)
{
    return tt::FCS::Checksum(data.data(), static_cast<int>(data.size()));
}

} // namespace

// Section 3 has a worked example: the octets 00 01 f2 03 f4 f5 f6 f7 give a sum of 2ddf0 before
// the carries are added back and ddf2 after. The checksum is the complement of that sum.
TEST(ChecksumTest, Rfc1071_Section3_NumericalExample)
{
    const std::vector<uint8_t> data = {0x00, 0x01, 0xf2, 0x03, 0xf4, 0xf5, 0xf6, 0xf7};
    EXPECT_EQ(0x2ddf0u, tt::FCS::ChecksumAdd(data.data(), static_cast<int>(data.size()), 0));
    EXPECT_EQ(static_cast<uint16_t>(~0xddf2), tt::FCS::ChecksumComplete(0x2ddf0));
    EXPECT_EQ(0x220d, Checksum(data));
}

// Section 2(A): the sum can be split into groups and the groups added in any order, as long as
// the even and odd octets stay where they are. The TCP pseudo header relies on this.
TEST(ChecksumTest, Rfc1071_Section2A_DataCanBeSummedInPieces)
{
    const std::vector<uint8_t> data = {0x00, 0x01, 0xf2, 0x03, 0xf4, 0xf5, 0xf6, 0xf7};
    uint32_t sum = tt::FCS::ChecksumAdd(data.data(), 4, 0);
    sum = tt::FCS::ChecksumAdd(data.data() + 4, 4, sum);
    EXPECT_EQ(Checksum(data), tt::FCS::ChecksumComplete(sum));
}

TEST(ChecksumTest, Rfc1071_Section2A_PiecesCanBeAddedInAnyOrder)
{
    std::mt19937 random(1071);
    for (int round = 0; round < 200; round++)
    {
        std::vector<uint8_t> a = RandomData(random, 2 * (random() % 50));
        std::vector<uint8_t> b = RandomData(random, 2 * (random() % 50));
        std::vector<uint8_t> c = RandomData(random, random() % 100); // only the last may be odd

        uint32_t forwards = tt::FCS::ChecksumAdd(a.data(), a.size(), 0);
        forwards = tt::FCS::ChecksumAdd(b.data(), b.size(), forwards);
        forwards = tt::FCS::ChecksumAdd(c.data(), c.size(), forwards);

        uint32_t backwards = tt::FCS::ChecksumAdd(b.data(), b.size(), 0);
        backwards = tt::FCS::ChecksumAdd(a.data(), a.size(), backwards);
        backwards = tt::FCS::ChecksumAdd(c.data(), c.size(), backwards);

        ASSERT_EQ(tt::FCS::ChecksumComplete(forwards), tt::FCS::ChecksumComplete(backwards));
    }
}

// Section 1: the checksum of nothing is the complement of a sum of zero.
TEST(ChecksumTest, Rfc1071_Section1_ChecksumOfNoDataIsAllOnes)
{
    EXPECT_EQ(0xffff, tt::FCS::Checksum(nullptr, 0));
}

// Section 1: when the number of octets is odd the last one is padded on the right with a zero
// octet, so it is the high order half of its 16 bit word.
TEST(ChecksumTest, Rfc1071_Section1_OddLastOctetIsPaddedOnTheRight)
{
    const std::vector<uint8_t> one = {0x12};
    EXPECT_EQ(static_cast<uint16_t>(~0x1200), Checksum(one));

    const std::vector<uint8_t> three = {0x00, 0x01, 0xab};
    EXPECT_EQ(static_cast<uint16_t>(~(0x0001 + 0xab00)), Checksum(three));
}

TEST(ChecksumTest, Rfc1071_Section1_OddDataGivesTheSameAsDataPaddedWithAZeroOctet)
{
    std::mt19937 random(1);
    for (int round = 0; round < 200; round++)
    {
        std::vector<uint8_t> odd = RandomData(random, 1 + 2 * (random() % 200));
        std::vector<uint8_t> padded = odd;
        padded.push_back(0);
        ASSERT_EQ(Checksum(padded), Checksum(odd)) << "length " << odd.size();
    }
}

// Section 2(C): each addition can produce a new carry, and every carry has to be added back.
// These inputs need the carries added back more than once.
TEST(ChecksumTest, Rfc1071_Section2C_CarriesAreAddedBackUntilNoneAreLeft)
{
    // ffff + ffff + 0001 = 1ffff. Adding back the carry gives 10000, which carries again.
    const std::vector<uint8_t> data = {0xff, 0xff, 0xff, 0xff, 0x00, 0x01};
    EXPECT_EQ(0x1ffffu, tt::FCS::ChecksumAdd(data.data(), 6, 0));
    EXPECT_EQ(0xfffe, tt::FCS::ChecksumComplete(0x1ffff));
    EXPECT_EQ(0xfffe, Checksum(data));

    EXPECT_EQ(0xfffe, tt::FCS::ChecksumComplete(0x10000));
    EXPECT_EQ(0x0000, tt::FCS::ChecksumComplete(0xffffffff));
    EXPECT_EQ(0xffff, tt::FCS::ChecksumComplete(0x00000000));
    EXPECT_EQ(0x0000, tt::FCS::ChecksumComplete(0x0000ffff));
}

TEST(ChecksumTest, Rfc1071_Section2C_AllOnesDataSumsToAllOnes)
{
    // Whatever the number of words, ffff added to itself is still ffff in one's complement
    for (size_t words : {1, 2, 3, 10, 100, 700})
    {
        std::vector<uint8_t> data(words * 2, 0xff);
        ASSERT_EQ(0x0000, Checksum(data)) << words << " words";
    }
}

TEST(ChecksumTest, Rfc1071_Section4_MatchesTheAlgorithmInTheRfc)
{
    std::mt19937 random(4);
    for (int round = 0; round < 5000; round++)
    {
        std::vector<uint8_t> data = RandomData(random, random() % 1600);
        ASSERT_EQ(ReferenceChecksum(data), Checksum(data)) << "length " << data.size();
    }
}

// Section 1: to check a checksum the sum is taken over the same octets including the checksum
// field. If the sum is all ones the check succeeds, which is a checksum of zero here.
TEST(ChecksumTest, Rfc1071_Section1_ReceiverCheckPassesWhenTheChecksumIsIncluded)
{
    std::mt19937 random(2);
    for (int round = 0; round < 2000; round++)
    {
        size_t length = 2 * (2 + random() % 700);
        std::vector<uint8_t> data = RandomData(random, length);

        // The checksum field is cleared, the checksum computed and stored, as for IP, UDP and TCP
        size_t field = 2 * (random() % (length / 2));
        data[field] = 0;
        data[field + 1] = 0;
        uint16_t checksum = Checksum(data);
        data[field] = checksum >> 8;
        data[field + 1] = checksum & 0xff;

        ASSERT_EQ(0x0000, Checksum(data)) << "length " << length << ", field at " << field;
    }
}

TEST(ChecksumTest, Rfc1071_Section1_ReceiverCheckFailsWhenADataBitChanges)
{
    std::mt19937 random(3);
    for (int round = 0; round < 500; round++)
    {
        size_t length = 2 * (2 + random() % 100);
        std::vector<uint8_t> data = RandomData(random, length);
        data[0] = 0;
        data[1] = 0;
        uint16_t checksum = Checksum(data);
        data[0] = checksum >> 8;
        data[1] = checksum & 0xff;
        ASSERT_EQ(0x0000, Checksum(data));

        data[random() % length] ^= static_cast<uint8_t>(1u << (random() % 8));
        ASSERT_NE(0x0000, Checksum(data)) << "length " << length;
    }
}

// Section 2(B): the sum can be taken in either byte order. Doing it on byte swapped data gives
// the byte swapped answer.
TEST(ChecksumTest, Rfc1071_Section2B_ByteOrderDoesNotMatter)
{
    std::mt19937 random(5);
    for (int round = 0; round < 1000; round++)
    {
        std::vector<uint8_t> data = RandomData(random, 2 * (1 + random() % 400));
        std::vector<uint8_t> swapped = data;
        for (size_t i = 0; i + 1 < swapped.size(); i += 2)
        {
            std::swap(swapped[i], swapped[i + 1]);
        }
        uint16_t normal = Checksum(data);
        uint16_t other = Checksum(swapped);
        ASSERT_EQ(static_cast<uint16_t>((normal << 8) | (normal >> 8)), other)
            << "length " << data.size();
    }
}

// A single flipped bit anywhere changes the checksum
TEST(ChecksumTest, Rfc1071_Section1_AnySingleBitChangeIsDetected)
{
    std::mt19937 random(6);
    std::vector<uint8_t> data = RandomData(random, 64);
    uint16_t original = Checksum(data);
    for (size_t byte = 0; byte < data.size(); byte++)
    {
        for (int bit = 0; bit < 8; bit++)
        {
            data[byte] ^= static_cast<uint8_t>(1u << bit);
            ASSERT_NE(original, Checksum(data)) << "byte " << byte << " bit " << bit;
            data[byte] ^= static_cast<uint8_t>(1u << bit);
        }
    }
}
