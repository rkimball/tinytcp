// Tests of ARP against RFC 826 (An Ethernet Address Resolution Protocol). Names are
// Rfc826_<requirement>.
//
// The packet reception algorithm of RFC 826 is, in order: discard the packet if the hardware type
// or the protocol type is not one we speak; if the pair <protocol type, sender protocol address>
// is already in the table, replace its hardware address with the sender's and note that it was
// merged; discard the packet if we are not the target protocol address; if it was not merged, add
// the sender to the table; and only now look at the opcode, answering a request with a reply.
#include <gtest/gtest.h>

#include "harness.hpp"

using namespace tt_test;

namespace
{

Bytes Frame(const ArpFields& arp,
            const Bytes& destination = StackMac,
            const Bytes& source = PeerMac,
            size_t padTo = 60)
{
    return EthernetFrame(destination, source, EtherTypeArp, ArpPacket(arp), padTo);
}

// The fields of the ARP packet in a frame that the stack sent
struct SentArp
{
    uint16_t hardwareType;
    uint16_t protocolType;
    uint8_t hardwareSize;
    uint8_t protocolSize;
    uint16_t opcode;
    Bytes senderMac;
    Bytes senderIp;
    Bytes targetMac;
    Bytes targetIp;
};

SentArp ParseArp(const Bytes& frame)
{
    const size_t at = 14; // after the Ethernet header
    SentArp arp;
    arp.hardwareType = Be16(frame, at);
    arp.protocolType = Be16(frame, at + 2);
    arp.hardwareSize = frame.at(at + 4);
    arp.protocolSize = frame.at(at + 5);
    arp.opcode = Be16(frame, at + 6);
    arp.senderMac = Slice(frame, at + 8, 6);
    arp.senderIp = Slice(frame, at + 14, 4);
    arp.targetMac = Slice(frame, at + 18, 6);
    arp.targetIp = Slice(frame, at + 24, 4);
    return arp;
}

// Whether the stack has an Ethernet address for ip. If it has, mac says what it is. Asking for an
// address the stack does not have makes it send an ARP request, so this clears what was sent.
bool Lookup(StackUnderTest& stack, const Bytes& ip, Bytes* mac = nullptr)
{
    const uint8_t* found = stack.stack().ARP.Protocol2Hardware(ip.data());
    if (found != nullptr && mac != nullptr)
    {
        *mac = Bytes(found, found + 6);
    }
    stack.ClearSent();
    return found != nullptr;
}

Bytes HostIp(int n)
{
    return Bytes({192, 168, 1, static_cast<uint8_t>(100 + n)});
}

Bytes HostMac(int n)
{
    return Bytes({0x02, 0x00, 0x5e, 0x00, 0x01, static_cast<uint8_t>(n)});
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Answering a request
// ---------------------------------------------------------------------------------------------

TEST(ArpTest, Rfc826_RequestForTheStacksAddressIsAnsweredWithAReply)
{
    StackUnderTest stack;
    stack.Receive(Frame(ArpRequest(PeerMac, PeerIp, StackIp), BroadcastMac));
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();

    // The reply has the fields of the request swapped: the stack's addresses are the sender and the
    // requester's are the target
    SentArp reply = ParseArp(stack.Sent()[0]);
    EXPECT_EQ(1, reply.hardwareType);
    EXPECT_EQ(0x0800, reply.protocolType);
    EXPECT_EQ(6, reply.hardwareSize);
    EXPECT_EQ(4, reply.protocolSize);
    EXPECT_EQ(2, reply.opcode);
    EXPECT_EQ(StackMac, reply.senderMac);
    EXPECT_EQ(StackIp, reply.senderIp);
    EXPECT_EQ(PeerMac, reply.targetMac);
    EXPECT_EQ(PeerIp, reply.targetIp);
}

TEST(ArpTest, Rfc826_ReplyGoesDirectlyToTheRequesterNotToEveryone)
{
    StackUnderTest stack;
    stack.Receive(Frame(ArpRequest(PeerMac, PeerIp, StackIp), BroadcastMac));
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    EXPECT_EQ(PeerMac, Slice(stack.Sent()[0], 0, 6)) << "Ethernet destination";
    EXPECT_EQ(StackMac, Slice(stack.Sent()[0], 6, 6)) << "Ethernet source";
    EXPECT_EQ(EtherTypeArp, Be16(stack.Sent()[0], 12));
}

TEST(ArpTest, Rfc826_RequestForAnotherHostIsNotAnswered)
{
    StackUnderTest stack;
    stack.Receive(Frame(ArpRequest(PeerMac, PeerIp, OtherIp), BroadcastMac));
    EXPECT_TRUE(stack.Sent().empty()) << stack.SentText();
}

TEST(ArpTest, Rfc826_ReplyIsNotAnsweredWithAReply)
{
    StackUnderTest stack;
    stack.Receive(Frame(ArpReply(PeerMac, PeerIp, StackMac, StackIp)));
    EXPECT_TRUE(stack.Sent().empty()) << stack.SentText();
}

TEST(ArpTest, Rfc826_UnknownOpcodesAreNotAnswered)
{
    for (uint16_t opcode : {0, 3, 4, 8, 0xffff})
    {
        StackUnderTest stack;
        ArpFields arp = ArpRequest(PeerMac, PeerIp, StackIp);
        arp.opcode = opcode;
        stack.Receive(Frame(arp, BroadcastMac));
        EXPECT_TRUE(stack.Sent().empty()) << "opcode " << opcode << "\n" << stack.SentText();
    }
}

TEST(ArpTest, Rfc826_RequestIsNotAnsweredBeforeTheStackHasAnAddress)
{
    // Before DHCP has given the stack an address it has no protocol address to be the target of
    StackUnderTest stack(StackUnderTest::MacOnly);
    stack.Receive(Frame(ArpRequest(PeerMac, PeerIp, ZeroIp), BroadcastMac));
    stack.Receive(Frame(ArpRequest(PeerMac, PeerIp, StackIp), BroadcastMac));
    EXPECT_TRUE(stack.Sent().empty()) << stack.SentText();
}

// ---------------------------------------------------------------------------------------------
// Discarding what the stack does not speak
// ---------------------------------------------------------------------------------------------

TEST(ArpTest, Rfc826_UnknownHardwareTypeIsDiscarded)
{
    for (uint16_t type : {0, 6, 0x0100, 0xffff})
    {
        StackUnderTest stack;
        ArpFields request = ArpRequest(PeerMac, PeerIp, StackIp);
        request.hardwareType = type;
        stack.Receive(Frame(request, BroadcastMac));
        EXPECT_TRUE(stack.Sent().empty()) << "hardware type " << type << "\n" << stack.SentText();

        ArpFields reply = ArpReply(PeerMac, PeerIp, StackMac, StackIp);
        reply.hardwareType = type;
        stack.Receive(Frame(reply));
        EXPECT_FALSE(Lookup(stack, PeerIp)) << "learned from hardware type " << type;
    }
}

TEST(ArpTest, Rfc826_UnknownProtocolTypeIsDiscarded)
{
    for (uint16_t type : {0x0000, 0x0806, 0x86dd, 0xffff})
    {
        StackUnderTest stack;
        ArpFields request = ArpRequest(PeerMac, PeerIp, StackIp);
        request.protocolType = type;
        stack.Receive(Frame(request, BroadcastMac));
        EXPECT_TRUE(stack.Sent().empty()) << "protocol type " << type << "\n" << stack.SentText();

        ArpFields reply = ArpReply(PeerMac, PeerIp, StackMac, StackIp);
        reply.protocolType = type;
        stack.Receive(Frame(reply));
        EXPECT_FALSE(Lookup(stack, PeerIp)) << "learned from protocol type " << type;
    }
}

TEST(ArpTest, Rfc826_UnexpectedAddressLengthsAreDiscarded)
{
    // RFC 826 allows the lengths to be checked. For Ethernet and IPv4 they are 6 and 4.
    const std::vector<std::pair<uint8_t, uint8_t>> lengths = {
        {8, 4}, {6, 16}, {0, 4}, {6, 0}, {255, 255}, {4, 6}};
    for (auto length : lengths)
    {
        StackUnderTest stack;
        ArpFields request = ArpRequest(PeerMac, PeerIp, StackIp);
        request.hardwareSize = length.first;
        request.protocolSize = length.second;
        stack.Receive(Frame(request, BroadcastMac));
        EXPECT_TRUE(stack.Sent().empty())
            << "lengths " << int(length.first) << " and " << int(length.second) << "\n"
            << stack.SentText();

        ArpFields reply = ArpReply(PeerMac, PeerIp, StackMac, StackIp);
        reply.hardwareSize = length.first;
        reply.protocolSize = length.second;
        stack.Receive(Frame(reply));
        EXPECT_FALSE(Lookup(stack, PeerIp))
            << "learned from lengths " << int(length.first) << " and " << int(length.second);
    }
}

TEST(ArpTest, Rfc826_PacketShorterThanAnArpPacketIsDiscarded)
{
    // An ARP packet for Ethernet and IPv4 is 28 octets. The stack must not read the missing octets
    // from whatever an earlier frame left in the buffer.
    StackUnderTest stack;
    UseEveryReceiveBuffer(stack);

    Bytes whole = ArpPacket(ArpRequest(PeerMac, PeerIp, StackIp));
    ASSERT_EQ(28u, whole.size());
    for (size_t length : {0, 4, 8, 14, 20, 27})
    {
        Bytes data(whole.begin(), whole.begin() + length);
        stack.Receive(EthernetFrame(BroadcastMac, PeerMac, EtherTypeArp, data, 0));
        EXPECT_TRUE(stack.Sent().empty()) << length << " octets of ARP\n" << stack.SentText();
        stack.ClearSent();
    }
}

// ---------------------------------------------------------------------------------------------
// The translation table
// ---------------------------------------------------------------------------------------------

TEST(ArpTest, Rfc826_ReplyToTheStackTeachesItTheSendersAddress)
{
    StackUnderTest stack;
    stack.Receive(Frame(ArpReply(PeerMac, PeerIp, StackMac, StackIp)));
    Bytes mac;
    ASSERT_TRUE(Lookup(stack, PeerIp, &mac));
    EXPECT_EQ(PeerMac, mac);
}

TEST(ArpTest, Rfc826_RequestForTheStackTeachesItTheSendersAddress)
{
    // The sender is added when the packet is for us, whatever the opcode
    StackUnderTest stack;
    stack.Receive(Frame(ArpRequest(PeerMac, PeerIp, StackIp), BroadcastMac));
    stack.ClearSent();
    Bytes mac;
    ASSERT_TRUE(Lookup(stack, PeerIp, &mac))
        << "the stack would have to ask for an address it was just told";
    EXPECT_EQ(PeerMac, mac);
}

TEST(ArpTest, Rfc826_ReplyForAnotherHostDoesNotAddAnEntry)
{
    StackUnderTest stack;
    stack.Receive(Frame(ArpReply(PeerMac, PeerIp, OtherMac, OtherIp), BroadcastMac));
    EXPECT_FALSE(Lookup(stack, PeerIp));
}

TEST(ArpTest, Rfc826_RequestForAnotherHostDoesNotAddAnEntry)
{
    StackUnderTest stack;
    stack.Receive(Frame(ArpRequest(PeerMac, PeerIp, OtherIp), BroadcastMac));
    EXPECT_FALSE(Lookup(stack, PeerIp));
}

TEST(ArpTest, Rfc826_NewHardwareAddressSupersedesTheOldOne)
{
    StackUnderTest stack;
    const Bytes newMac = {0x02, 0x00, 0x5e, 0x00, 0x00, 0x99};
    stack.Receive(Frame(ArpReply(PeerMac, PeerIp, StackMac, StackIp)));
    stack.Receive(Frame(ArpReply(newMac, PeerIp, StackMac, StackIp), StackMac, newMac));
    Bytes mac;
    ASSERT_TRUE(Lookup(stack, PeerIp, &mac));
    EXPECT_EQ(newMac, mac);
}

TEST(ArpTest, Rfc826_ExistingEntryIsUpdatedEvenByAPacketForAnotherHost)
{
    // The merge comes before the question of whether the packet is for us
    StackUnderTest stack;
    const Bytes newMac = {0x02, 0x00, 0x5e, 0x00, 0x00, 0x99};
    TeachArp(stack, PeerMac, PeerIp);
    stack.Receive(Frame(ArpRequest(newMac, PeerIp, OtherIp), BroadcastMac, newMac));
    Bytes mac;
    ASSERT_TRUE(Lookup(stack, PeerIp, &mac));
    EXPECT_EQ(newMac, mac);
}

TEST(ArpTest, Rfc826_AnAddressThatIsKnownIsNotAskedFor)
{
    StackUnderTest stack;
    TeachArp(stack, PeerMac, PeerIp);
    const uint8_t* found = stack.stack().ARP.Protocol2Hardware(PeerIp.data());
    ASSERT_NE(nullptr, found);
    EXPECT_EQ(PeerMac, Bytes(found, found + 6));
    EXPECT_TRUE(stack.Sent().empty()) << stack.SentText();
}

TEST(ArpTest, Rfc826_TableHoldsSeveralEntries)
{
    StackUnderTest stack;
    for (int n = 0; n < tt::ARPCacheSize; n++)
    {
        TeachArp(stack, HostMac(n), HostIp(n));
    }
    for (int n = 0; n < tt::ARPCacheSize; n++)
    {
        Bytes mac;
        ASSERT_TRUE(Lookup(stack, HostIp(n), &mac)) << "host " << n;
        EXPECT_EQ(HostMac(n), mac) << "host " << n;
    }
}

TEST(ArpTest, Rfc826_FullTableMakesRoomForTheNewestEntry)
{
    // RFC 826 does not say what to do when the table is full. This stack replaces the oldest entry.
    StackUnderTest stack;
    const int hosts = tt::ARPCacheSize + 1;
    for (int n = 0; n < hosts; n++)
    {
        TeachArp(stack, HostMac(n), HostIp(n));
    }

    Bytes mac;
    ASSERT_TRUE(Lookup(stack, HostIp(hosts - 1), &mac)) << "the newest host";
    EXPECT_EQ(HostMac(hosts - 1), mac);
    EXPECT_FALSE(Lookup(stack, HostIp(0))) << "the oldest host was replaced";
    for (int n = 1; n < hosts; n++)
    {
        ASSERT_TRUE(Lookup(stack, HostIp(n), &mac)) << "host " << n;
        EXPECT_EQ(HostMac(n), mac) << "host " << n;
    }
}

// ---------------------------------------------------------------------------------------------
// Asking for an address
// ---------------------------------------------------------------------------------------------

TEST(ArpTest, Rfc826_RequestForAnUnknownAddressIsBroadcast)
{
    StackUnderTest stack;
    EXPECT_EQ(nullptr, stack.stack().ARP.Protocol2Hardware(OtherIp.data()));
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    const Bytes& frame = stack.Sent()[0];

    EXPECT_EQ(BroadcastMac, Slice(frame, 0, 6)) << "Ethernet destination";
    EXPECT_EQ(StackMac, Slice(frame, 6, 6)) << "Ethernet source";
    EXPECT_EQ(EtherTypeArp, Be16(frame, 12));
}

TEST(ArpTest, Rfc826_RequestHasTheFieldsRfc826Gives)
{
    StackUnderTest stack;
    stack.stack().ARP.Protocol2Hardware(OtherIp.data());
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();

    SentArp request = ParseArp(stack.Sent()[0]);
    EXPECT_EQ(1, request.hardwareType) << "Ethernet";
    EXPECT_EQ(0x0800, request.protocolType) << "IP";
    EXPECT_EQ(6, request.hardwareSize);
    EXPECT_EQ(4, request.protocolSize);
    EXPECT_EQ(1, request.opcode) << "request";
    EXPECT_EQ(StackMac, request.senderMac);
    EXPECT_EQ(StackIp, request.senderIp);
    EXPECT_EQ(OtherIp, request.targetIp);
    // The target hardware address is what is being asked for. RFC 826 leaves it unspecified in a
    // request, and zeros or the broadcast address are both usual.
    EXPECT_TRUE(request.targetMac == ZeroMac || request.targetMac == BroadcastMac)
        << Hex(request.targetMac);
}

TEST(ArpTest, Rfc826_PacketWaitingForAnAddressIsSentWhenTheReplyArrives)
{
    // The peer pings the stack, which does not know the peer's Ethernet address yet. It asks, and
    // sends the echo reply when the answer comes.
    StackUnderTest stack;
    const Bytes data = {9, 8, 7, 6, 5, 4, 3, 2};
    stack.Receive(EthernetFrame(StackMac,
                                PeerMac,
                                EtherTypeIp,
                                Ipv4Packet(PeerIp, StackIp, 1, IcmpEcho(8, 0x4321, 3, data))));

    ASSERT_EQ(1u, stack.Sent().size()) << "only the ARP request so far\n" << stack.SentText();
    SentArp request = ParseArp(stack.Sent()[0]);
    EXPECT_EQ(1, request.opcode);
    EXPECT_EQ(PeerIp, request.targetIp);
    stack.ClearSent();

    stack.Receive(Frame(ArpReply(PeerMac, PeerIp, StackMac, StackIp)));
    ASSERT_EQ(1u, stack.Sent().size()) << "the echo reply\n" << stack.SentText();
    const Bytes& frame = stack.Sent()[0];
    EXPECT_EQ(PeerMac, Slice(frame, 0, 6));
    EXPECT_EQ(EtherTypeIp, Be16(frame, 12));
    EXPECT_EQ(PeerIp, Slice(frame, 14 + 16, 4)) << "IP destination";
    EXPECT_EQ(0, frame.at(14 + 20)) << "ICMP type of an echo reply";
    EXPECT_EQ(data, Slice(frame, 14 + 20 + 8, 8));
}
