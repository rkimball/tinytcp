// Tests of ICMP against RFC 792 (Internet Control Message Protocol) and RFC 1122 section 3.2.2.
// Names are Rfc<n>_<requirement>.
#include <random>

#include <gtest/gtest.h>

#include "harness.hpp"

using namespace tt_test;

namespace
{

// An ICMP message with a correct checksum
Bytes IcmpMessage(uint8_t type, uint8_t code, const Bytes& rest)
{
    Bytes message;
    message.push_back(type);
    message.push_back(code);
    Append16(message, 0);
    Append(message, rest);
    uint16_t checksum = InternetChecksum(message);
    message[2] = static_cast<uint8_t>(checksum >> 8);
    message[3] = static_cast<uint8_t>(checksum & 0xff);
    return message;
}

Ipv4Fields Datagram(const Bytes& icmp)
{
    Ipv4Fields ip;
    ip.protocol = 1;
    ip.payload = icmp;
    return ip;
}

Bytes Frame(const Ipv4Fields& ip,
            const Bytes& destinationMac = StackMac,
            const Bytes& sourceMac = PeerMac,
            size_t padTo = 60)
{
    return EthernetFrame(destinationMac, sourceMac, EtherTypeIp, Ipv4Datagram(ip), padTo);
}

// What the stack sends when the datagram arrives and the stack already knows the peer
std::vector<Bytes>
    SentAfter(const Ipv4Fields& ip, const Bytes& destinationMac = StackMac, size_t padTo = 60)
{
    StackUnderTest stack;
    TeachArp(stack, PeerMac, PeerIp);
    stack.Receive(Frame(ip, destinationMac, PeerMac, padTo));
    return stack.Sent();
}

// The ICMP message in a frame that the stack sent, as long as the IP header says it is
Bytes IcmpOf(const Bytes& frame)
{
    size_t total = Be16(frame, 14 + 2);
    return Slice(frame, 14 + 20, total - 20);
}

Bytes Echo(uint8_t type, uint16_t identifier, uint16_t sequence, const Bytes& data)
{
    return IcmpEcho(type, identifier, sequence, data);
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Echo
// ---------------------------------------------------------------------------------------------

TEST(IcmpTest, Rfc792_EchoRequestIsAnsweredWithAnEchoReply)
{
    const Bytes data = {1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<Bytes> sent = SentAfter(Datagram(Echo(8, 0x1234, 7, data)));
    ASSERT_EQ(1u, sent.size());
    const Bytes& frame = sent[0];

    EXPECT_EQ(1, frame.at(14 + 9)) << "IP protocol is ICMP";
    EXPECT_EQ(StackIp, Slice(frame, 14 + 12, 4)) << "from the stack";
    EXPECT_EQ(PeerIp, Slice(frame, 14 + 16, 4)) << "to the host that asked";
    EXPECT_EQ(PeerMac, Slice(frame, 0, 6));

    Bytes icmp = IcmpOf(frame);
    ASSERT_EQ(8u + data.size(), icmp.size());
    EXPECT_EQ(0, icmp[0]) << "type 0 is an echo reply";
    EXPECT_EQ(0, icmp[1]) << "code";
    EXPECT_EQ(0x1234, Be16(icmp, 4)) << "identifier";
    EXPECT_EQ(7, Be16(icmp, 6)) << "sequence number";
    EXPECT_EQ(data, Slice(icmp, 8, data.size()));
}

TEST(IcmpTest, Rfc1122_DataOfAnEchoRequestIsEntirelyInTheReply)
{
    // Odd and even lengths, from nothing up to what fits in a 576 octet datagram
    std::mt19937 random(792);
    for (size_t length : {0, 1, 2, 3, 7, 8, 9, 100, 255, 256, 547, 548})
    {
        Bytes data(length);
        for (auto& byte : data)
        {
            byte = static_cast<uint8_t>(random());
        }
        std::vector<Bytes> sent = SentAfter(Datagram(Echo(8, 0x42, 9, data)));
        ASSERT_EQ(1u, sent.size()) << length << " octets of data";
        Bytes icmp = IcmpOf(sent[0]);
        ASSERT_EQ(8 + length, icmp.size()) << length << " octets of data";
        EXPECT_EQ(data, Slice(icmp, 8, length)) << length << " octets of data";
    }
}

TEST(IcmpTest, Rfc792_IdentifierAndSequenceNumberAreReturnedUnchanged)
{
    for (uint16_t value : {0, 1, 0x00ff, 0x0100, 0x1234, 0x8000, 0xffff})
    {
        std::vector<Bytes> sent = SentAfter(Datagram(Echo(8, value, ~value & 0xffff, {})));
        ASSERT_EQ(1u, sent.size());
        Bytes icmp = IcmpOf(sent[0]);
        EXPECT_EQ(value, Be16(icmp, 4)) << "identifier";
        EXPECT_EQ(~value & 0xffff, Be16(icmp, 6)) << "sequence number";
    }
}

TEST(IcmpTest, Rfc792_ChecksumOfTheEchoReplyCoversTheWholeMessage)
{
    // The checksum over a message that has a right checksum in it comes to zero. That includes
    // messages with an odd number of octets, which are padded with a zero octet for the sum.
    std::mt19937 random(8);
    for (int round = 0; round < 300; round++)
    {
        Bytes data(random() % 548);
        for (auto& byte : data)
        {
            byte = static_cast<uint8_t>(random());
        }
        std::vector<Bytes> sent = SentAfter(Datagram(Echo(8, 1, 1, data)));
        ASSERT_EQ(1u, sent.size()) << data.size() << " octets of data";
        ASSERT_EQ(0, InternetChecksum(IcmpOf(sent[0]))) << data.size() << " octets of data";
    }
}

TEST(IcmpTest, Rfc792_MessageWithABadChecksumIsDiscarded)
{
    // RFC 792 defines the checksum over the whole message, so one that does not match is damaged
    const Bytes good = Echo(8, 0x1234, 7, {1, 2, 3, 4, 5, 6, 7, 8});
    for (size_t bit = 0; bit < good.size() * 8; bit++)
    {
        Bytes message = good;
        message[bit / 8] ^= static_cast<uint8_t>(1u << (bit % 8));
        EXPECT_TRUE(SentAfter(Datagram(message)).empty()) << "bit " << bit << " flipped";
    }
}

TEST(IcmpTest, Rfc792_MessageShorterThanAnEchoHeaderIsDiscarded)
{
    // An echo message has 8 octets before the data: type, code, checksum, identifier and sequence
    const Bytes whole = Echo(8, 0x1234, 7, {});
    for (size_t length = 0; length < 8; length++)
    {
        Bytes message = Slice(whole, 0, length);
        if (length >= 4)
        {
            // keep the checksum right for what is there
            message[2] = 0;
            message[3] = 0;
            uint16_t checksum = InternetChecksum(message);
            message[2] = static_cast<uint8_t>(checksum >> 8);
            message[3] = static_cast<uint8_t>(checksum & 0xff);
        }
        EXPECT_TRUE(SentAfter(Datagram(message)).empty()) << length << " octets";
    }
}

TEST(IcmpTest, Rfc1122_UnknownIcmpTypesAreSilentlyDiscarded)
{
    // The stack answers an echo request and nothing else. This takes in the replies and the error
    // messages, which must never be answered.
    StackUnderTest stack;
    TeachArp(stack, PeerMac, PeerIp);
    for (int type = 0; type < 256; type++)
    {
        if (type == 8)
        {
            continue;
        }
        stack.Receive(Frame(Datagram(IcmpMessage(static_cast<uint8_t>(type), 0, Bytes(12, 0x33)))));
        ASSERT_TRUE(stack.Sent().empty()) << "type " << type << "\n" << stack.SentText();
    }
}

// ---------------------------------------------------------------------------------------------
// Destination unreachable
// ---------------------------------------------------------------------------------------------

TEST(IcmpTest, Rfc1122_UnsupportedProtocolIsAnsweredWithProtocolUnreachable)
{
    for (uint8_t protocol : {99, 132, 253})
    {
        Ipv4Fields ip;
        ip.protocol = protocol;
        ip.payload = Bytes({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
        std::vector<Bytes> sent = SentAfter(ip);
        ASSERT_EQ(1u, sent.size()) << "protocol " << int(protocol);
        const Bytes& frame = sent[0];

        EXPECT_EQ(1, frame.at(14 + 9)) << "IP protocol is ICMP";
        EXPECT_EQ(StackIp, Slice(frame, 14 + 12, 4));
        EXPECT_EQ(PeerIp, Slice(frame, 14 + 16, 4));

        Bytes icmp = IcmpOf(frame);
        ASSERT_EQ(8u + 20 + 8, icmp.size()) << "the original header and 8 octets of its data";
        EXPECT_EQ(3, icmp[0]) << "destination unreachable";
        EXPECT_EQ(2, icmp[1]) << "protocol unreachable";
        EXPECT_EQ(Bytes(4, 0), Slice(icmp, 4, 4)) << "unused, and zero when sent";
        EXPECT_EQ(Slice(Ipv4Datagram(ip), 0, 28), Slice(icmp, 8, 28))
            << "the datagram that caused it";
        EXPECT_EQ(0, InternetChecksum(icmp));
    }
}

TEST(IcmpTest, Rfc792_ShortDatagramIsQuotedAsFarAsItGoes)
{
    Ipv4Fields ip;
    ip.protocol = 99;
    ip.payload = Bytes({1, 2, 3});
    std::vector<Bytes> sent = SentAfter(ip);
    ASSERT_EQ(1u, sent.size());
    Bytes icmp = IcmpOf(sent[0]);
    ASSERT_EQ(8u + 20 + 3, icmp.size());
    EXPECT_EQ(Ipv4Datagram(ip), Slice(icmp, 8, 23));
    EXPECT_EQ(0, InternetChecksum(icmp));
}

TEST(IcmpTest, Rfc1122_NoErrorIsSentForADatagramToABroadcastAddress)
{
    for (const Bytes& destination :
         {Bytes({255, 255, 255, 255}), SubnetBroadcastIp, Bytes({224, 0, 0, 1})})
    {
        Ipv4Fields ip;
        ip.protocol = 99;
        ip.destination = destination;
        ip.payload = Bytes(12, 0x22);
        EXPECT_TRUE(SentAfter(ip, BroadcastMac).empty()) << "to " << Hex(destination);
    }
}

TEST(IcmpTest, Rfc1122_NoErrorIsSentForADatagramInALinkLayerBroadcastFrame)
{
    Ipv4Fields ip;
    ip.protocol = 99;
    ip.payload = Bytes(12, 0x22);
    EXPECT_TRUE(SentAfter(ip, BroadcastMac).empty());
}

TEST(IcmpTest, Rfc1122_NoErrorIsSentForAFragmentThatIsNotTheFirst)
{
    Ipv4Fields ip;
    ip.protocol = 99;
    ip.flagsAndFragmentOffset = 0x00b9;
    ip.payload = Bytes(12, 0x22);
    EXPECT_TRUE(SentAfter(ip).empty());
}

TEST(IcmpTest, Rfc1122_NoErrorIsSentToASourceThatIsNotASingleHost)
{
    // Zero, broadcast, multicast and class E addresses do not name one host
    const std::vector<Bytes> sources = {ZeroIp,
                                        Bytes({255, 255, 255, 255}),
                                        SubnetBroadcastIp,
                                        Bytes({127, 0, 0, 1}),
                                        Bytes({224, 0, 0, 1}),
                                        Bytes({240, 0, 0, 1}),
                                        Bytes({255, 255, 255, 254})};
    for (const Bytes& source : sources)
    {
        Ipv4Fields ip;
        ip.protocol = 99;
        ip.source = source;
        ip.payload = Bytes(12, 0x22);
        EXPECT_TRUE(SentAfter(ip).empty()) << "from " << Hex(source);
    }
}
