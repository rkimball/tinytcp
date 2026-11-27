// Tests of the Ethernet layer against RFC 894 (A Standard for the Transmission of IP Datagrams
// over Ethernet Networks). Names are Rfc894_<requirement>.
//
// RFC 894 says: the type field is 0800 for IP, the data field is 46 to 1500 octets, short data is
// padded with zero octets and the padding is not part of the IP datagram, and the broadcast IP
// address maps to the Ethernet broadcast address ff-ff-ff-ff-ff-ff. Frames for ARP use the type
// 0806 (RFC 826 and the IANA registry of Ethernet types).
#include <gtest/gtest.h>

#include "harness.hpp"

using namespace tt_test;

namespace
{

// An ARP request for the stack's own address. The stack answers it with an ARP reply.
Bytes RequestForTheStack(const Bytes& destination = StackMac,
                         size_t padTo = 60,
                         uint16_t type = EtherTypeArp)
{
    return EthernetFrame(
        destination, PeerMac, type, ArpPacket(ArpRequest(PeerMac, PeerIp, StackIp)), padTo);
}

const Bytes EchoData = {1, 2, 3, 4, 5, 6, 7, 8};

// An ICMP echo request to the stack, which the stack answers with an echo reply
Bytes EchoRequestForTheStack(uint16_t type = EtherTypeIp, size_t padTo = 60)
{
    return EthernetFrame(StackMac,
                         PeerMac,
                         type,
                         Ipv4Packet(PeerIp, StackIp, 1, IcmpEcho(8, 0x1234, 7, EchoData)),
                         padTo);
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Receiving
// ---------------------------------------------------------------------------------------------

TEST(EthernetTest, Rfc894_TypeFieldOf0800IsAnIpDatagram)
{
    StackUnderTest stack;
    TeachArp(stack, PeerMac, PeerIp);
    stack.Receive(EchoRequestForTheStack());
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    EXPECT_EQ(EtherTypeIp, Be16(stack.Sent()[0], 12));
}

TEST(EthernetTest, Rfc894_TypeFieldOf0806IsAnArpPacket)
{
    StackUnderTest stack;
    stack.Receive(RequestForTheStack());
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    EXPECT_EQ(EtherTypeArp, Be16(stack.Sent()[0], 12));
}

TEST(EthernetTest, Rfc894_OtherTypeValuesAreNeitherIpNorArp)
{
    for (uint16_t type : {0x0000, 0x0200, 0x0801, 0x0807, 0x0842, 0x8035, 0x8100, 0x86dd, 0xffff})
    {
        StackUnderTest stack;
        TeachArp(stack, PeerMac, PeerIp);
        stack.Receive(EchoRequestForTheStack(type));
        stack.Receive(RequestForTheStack(StackMac, 60, type));
        EXPECT_TRUE(stack.Sent().empty()) << "type " << std::hex << type << "\n"
                                          << stack.SentText();
    }
}

TEST(EthernetTest, Rfc894_FramesAddressedToTheStackAreAccepted)
{
    StackUnderTest stack;
    stack.Receive(RequestForTheStack(StackMac));
    EXPECT_EQ(1u, stack.Sent().size()) << stack.SentText();
}

TEST(EthernetTest, Rfc894_BroadcastFramesAreAccepted)
{
    StackUnderTest stack;
    stack.Receive(RequestForTheStack(BroadcastMac));
    EXPECT_EQ(1u, stack.Sent().size()) << stack.SentText();
}

TEST(EthernetTest, Rfc894_FramesAddressedToOtherStationsAreIgnored)
{
    const std::vector<Bytes> others = {
        OtherMac,
        {0x01, 0x00, 0x5e, 0x00, 0x00, 0xfb}, // an IP multicast group
        {0x02, 0x00, 0x5e, 0x00, 0x00, 0x00}, // one octet off from the stack's address
        {0x03, 0x00, 0x5e, 0x00, 0x00, 0x01}, // one bit off from the stack's address
        ZeroMac,
    };
    for (const Bytes& other : others)
    {
        StackUnderTest stack;
        stack.Receive(RequestForTheStack(other));
        EXPECT_TRUE(stack.Sent().empty()) << Hex(other) << "\n" << stack.SentText();
    }
}

TEST(EthernetTest, Rfc894_ZeroPaddingAfterTheDataIsIgnored)
{
    // An ARP packet is 28 octets and the frame is padded to the minimum
    StackUnderTest stack;
    stack.Receive(RequestForTheStack(StackMac, 60));
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    EXPECT_EQ(2, Be16(stack.Sent()[0], 14 + 6)) << "opcode of the reply";
}

TEST(EthernetTest, Rfc894_PaddingOfAnyValueIsIgnored)
{
    StackUnderTest zeros;
    zeros.Receive(RequestForTheStack(StackMac, 60));
    ASSERT_EQ(1u, zeros.Sent().size());

    for (uint8_t fill : {0x00, 0x01, 0x55, 0xaa, 0xff})
    {
        for (size_t length : {60, 61, 100, 300})
        {
            Bytes frame = RequestForTheStack(StackMac, 42); // the frame, without padding
            frame.resize(length, fill);
            StackUnderTest stack;
            stack.Receive(frame);
            ASSERT_EQ(1u, stack.Sent().size()) << "fill " << int(fill) << " length " << length;
            EXPECT_EQ(zeros.Sent()[0], stack.Sent()[0])
                << "fill " << int(fill) << " length " << length;
        }
    }
}

TEST(EthernetTest, Rfc894_PaddingIsNotPartOfTheIpDatagram)
{
    // The echo request is 14 + 20 + 8 + 8 = 50 octets, so the frame is padded to 60. The padding is
    // not in the total length field of the IP header, so the echo reply carries 8 octets of data.
    StackUnderTest stack;
    TeachArp(stack, PeerMac, PeerIp);
    stack.Receive(EchoRequestForTheStack());
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    const Bytes& reply = stack.Sent()[0];

    EXPECT_EQ(20 + 8 + 8, Be16(reply, 14 + 2)) << "total length of the reply";
    EXPECT_EQ(EchoData, Slice(reply, 14 + 20 + 8, 8)) << "data of the reply";
}

TEST(EthernetTest, Rfc894_FramesShorterThanAnEthernetHeaderAreIgnored)
{
    // A frame of fewer than 14 octets has no type field. The stack must not read the missing
    // octets from whatever an earlier frame left in the buffer.
    StackUnderTest stack;
    UseEveryReceiveBuffer(stack);

    Bytes request = RequestForTheStack();
    for (size_t length = 0; length < 14; length++)
    {
        Bytes runt(request.begin(), request.begin() + length);
        stack.Receive(runt);
        EXPECT_TRUE(stack.Sent().empty()) << "a frame of " << length << " octets\n"
                                          << stack.SentText();
        stack.ClearSent();
    }
}

TEST(EthernetTest, Rfc894_FramesUpToTheLargestDatagramThatMustBeAcceptedAreReceived)
{
    // RFC 791 says every host must accept a datagram of 576 octets, so a frame of 14 + 576
    StackUnderTest stack;
    stack.Receive(RequestForTheStack(StackMac, 14 + 576));
    EXPECT_EQ(1u, stack.Sent().size()) << "a frame of " << 14 + 576 << " octets";
}

TEST(EthernetTest, Rfc894_AFrameThatFillsTheReceiveBufferIsReceived)
{
    StackUnderTest stack;
    stack.Receive(RequestForTheStack(StackMac, DATA_BUFFER_PAYLOAD_SIZE));
    EXPECT_EQ(1u, stack.Sent().size()) << "a frame of " << DATA_BUFFER_PAYLOAD_SIZE << " octets";
}

TEST(EthernetTest, Rfc894_FramesLargerThanTheReceiveBuffersAreDropped)
{
    // The largest frame RFC 894 allows is 14 + 1500 octets. The receive buffers are smaller, so a
    // frame that does not fit is dropped whole. Peers do not send them, because without a
    // maximum segment size option TCP sends segments of at most 536 octets.
    StackUnderTest stack;
    stack.Receive(RequestForTheStack(StackMac, DATA_BUFFER_PAYLOAD_SIZE + 1));
    stack.Receive(RequestForTheStack(StackMac, 14 + 1500));
    EXPECT_TRUE(stack.Sent().empty()) << stack.SentText();

    // And nothing was damaged: the next ordinary frame is handled
    stack.Receive(RequestForTheStack());
    EXPECT_EQ(1u, stack.Sent().size()) << stack.SentText();
}

// ---------------------------------------------------------------------------------------------
// Transmitting
// ---------------------------------------------------------------------------------------------

TEST(EthernetTest, Rfc894_FrameHasDestinationSourceTypeThenData)
{
    StackUnderTest stack;
    stack.Receive(RequestForTheStack());
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    const Bytes& frame = stack.Sent()[0];

    EXPECT_EQ(PeerMac, Slice(frame, 0, 6)) << "destination";
    EXPECT_EQ(StackMac, Slice(frame, 6, 6)) << "source";
    EXPECT_EQ(EtherTypeArp, Be16(frame, 12)) << "type";
    EXPECT_EQ(1, Be16(frame, 14)) << "the data starts with the ARP hardware type";
}

TEST(EthernetTest, Rfc894_ShortFramesAreSentPaddedWithZeroOctetsToTheMinimum)
{
    // 14 octets of header and 46 of data make the 60 octet minimum. An ARP reply is 28 octets.
    StackUnderTest stack;
    stack.Receive(RequestForTheStack());
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    const Bytes& frame = stack.Sent()[0];

    ASSERT_EQ(60u, frame.size());
    EXPECT_EQ(Bytes(60 - 14 - 28, 0), Slice(frame, 14 + 28, 60 - 14 - 28)) << "the padding";
}

TEST(EthernetTest, Rfc894_FramesThatAreLongEnoughAreNotPadded)
{
    // The DHCP discover is well over 46 octets of data
    StackUnderTest stack;
    stack.stack().StartDHCP();
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    const Bytes& frame = stack.Sent()[0];

    EXPECT_GT(frame.size(), 60u);
    EXPECT_EQ(14u + Be16(frame, 14 + 2), frame.size())
        << "the frame is the header plus the datagram";
}

TEST(EthernetTest, Rfc894_BroadcastIpAddressIsSentToTheBroadcastEthernetAddress)
{
    // The DHCP discover goes to 255.255.255.255
    StackUnderTest stack;
    stack.stack().StartDHCP();
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    const Bytes& frame = stack.Sent()[0];

    EXPECT_EQ(Bytes({255, 255, 255, 255}), Slice(frame, 14 + 16, 4)) << "destination IP address";
    EXPECT_EQ(BroadcastMac, Slice(frame, 0, 6)) << "destination Ethernet address";
}

TEST(EthernetTest, Rfc894_EveryFrameSentIsBetween60And1514Octets)
{
    StackUnderTest stack;
    stack.Receive(RequestForTheStack());                 // an ARP reply
    stack.stack().ARP.Protocol2Hardware(OtherIp.data()); // an ARP request
    stack.stack().StartDHCP();                           // a DHCP discover
    ASSERT_EQ(3u, stack.Sent().size()) << stack.SentText();
    for (const Bytes& frame : stack.Sent())
    {
        EXPECT_GE(frame.size(), 60u);
        EXPECT_LE(frame.size(), 14u + 1500u);
    }
}
