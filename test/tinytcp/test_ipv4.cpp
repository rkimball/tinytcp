// Tests of the IPv4 layer against RFC 791 (Internet Protocol), the IP parts of RFC 1122
// (Requirements for Internet Hosts), RFC 919 and RFC 922 (broadcasting), RFC 950 (subnets), and RFC
// 2474 and RFC 3168 (the DS and ECN bits). Names are Rfc<n>_<requirement>.
//
// The datagram that carries the test is an ICMP echo request, because the stack answers one with an
// echo reply, which shows that the datagram was accepted.
#include <gtest/gtest.h>

#include "harness.hpp"

using namespace tt_test;

namespace
{

const Bytes EchoData = {1, 2, 3, 4, 5, 6, 7, 8};
const Bytes OffSubnetIp = {10, 0, 0, 5};
const Bytes GatewayMac = {0x02, 0x00, 0x5e, 0x00, 0x00, 0x10};

// A datagram from the peer to the stack that carries an ICMP echo request
Ipv4Fields EchoRequest()
{
    Ipv4Fields ip;
    ip.protocol = 1;
    ip.payload = IcmpEcho(8, 0x1234, 7, EchoData);
    return ip;
}

Bytes Frame(const Ipv4Fields& ip,
            const Bytes& destinationMac = StackMac,
            const Bytes& sourceMac = PeerMac,
            size_t padTo = 60)
{
    return EthernetFrame(destinationMac, sourceMac, EtherTypeIp, Ipv4Datagram(ip), padTo);
}

// Give the datagram to a stack that already knows the peer's Ethernet address and return what the
// stack sent in answer
std::vector<Bytes>
    SentAfter(const Ipv4Fields& ip, const Bytes& destinationMac = StackMac, size_t padTo = 60)
{
    StackUnderTest stack;
    TeachArp(stack, PeerMac, PeerIp);
    stack.Receive(Frame(ip, destinationMac, PeerMac, padTo));
    return stack.Sent();
}

// The fields of the IPv4 header of a frame that the stack sent
struct SentIp
{
    uint8_t versionAndHeaderLength;
    uint8_t typeOfService;
    uint16_t totalLength;
    uint16_t id;
    uint16_t flagsAndFragmentOffset;
    uint8_t ttl;
    uint8_t protocol;
    Bytes source;
    Bytes destination;
    Bytes header;
};

SentIp ParseIp(const Bytes& frame)
{
    const size_t at = 14; // after the Ethernet header
    SentIp ip;
    ip.versionAndHeaderLength = frame.at(at);
    ip.typeOfService = frame.at(at + 1);
    ip.totalLength = Be16(frame, at + 2);
    ip.id = Be16(frame, at + 4);
    ip.flagsAndFragmentOffset = Be16(frame, at + 6);
    ip.ttl = frame.at(at + 8);
    ip.protocol = frame.at(at + 9);
    ip.source = Slice(frame, at + 12, 4);
    ip.destination = Slice(frame, at + 16, 4);
    ip.header = Slice(frame, at, 20);
    return ip;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// What a received datagram has to be to be accepted
// ---------------------------------------------------------------------------------------------

TEST(Ipv4Test, ADatagramThatIsRightIsAccepted)
{
    EXPECT_EQ(1u, SentAfter(EchoRequest()).size());
}

TEST(Ipv4Test, Rfc1122_DatagramWithAVersionOtherThan4IsDiscarded)
{
    for (int version = 0; version < 16; version++)
    {
        if (version == 4)
        {
            continue;
        }
        Ipv4Fields ip = EchoRequest();
        ip.versionAndHeaderLength = (version << 4) | 5;
        EXPECT_TRUE(SentAfter(ip).empty()) << "version " << version;
    }
}

TEST(Ipv4Test, Rfc791_HeaderLengthOfLessThan5WordsIsDiscarded)
{
    for (int words = 0; words < 5; words++)
    {
        Ipv4Fields ip = EchoRequest();
        ip.versionAndHeaderLength = 0x40 | words;
        EXPECT_TRUE(SentAfter(ip).empty()) << words << " words";
    }
}

TEST(Ipv4Test, Rfc791_TotalLengthShorterThanTheHeaderIsDiscarded)
{
    for (int length : {0, 1, 19})
    {
        Ipv4Fields ip = EchoRequest();
        ip.totalLength = length;
        EXPECT_TRUE(SentAfter(ip).empty()) << "total length " << length;
    }
}

TEST(Ipv4Test, Rfc791_TotalLengthLongerThanWhatArrivedIsDiscarded)
{
    // A frame with no padding, so what arrived is the datagram and nothing else
    const Ipv4Fields echo = EchoRequest();
    const size_t actual = Ipv4Datagram(echo).size();
    for (size_t extra : {1, 10, 100, 60000})
    {
        Ipv4Fields ip = echo;
        ip.totalLength = static_cast<int>(actual + extra);
        EXPECT_TRUE(SentAfter(ip, StackMac, 0).empty()) << "claims " << extra << " octets too many";
    }
}

TEST(Ipv4Test, Rfc1122_HeaderChecksumIsVerified)
{
    // Flipping any one bit of the header, the checksum field included, makes the checksum wrong
    const Bytes good = Ipv4Datagram(EchoRequest());
    for (size_t bit = 0; bit < 20 * 8; bit++)
    {
        Ipv4Fields ip = EchoRequest();
        Bytes header = Slice(good, 0, 20);
        header[bit / 8] ^= static_cast<uint8_t>(1u << (bit % 8));

        StackUnderTest stack;
        TeachArp(stack, PeerMac, PeerIp);
        Bytes datagram = header;
        Append(datagram, ip.payload);
        stack.Receive(EthernetFrame(StackMac, PeerMac, EtherTypeIp, datagram));
        ASSERT_TRUE(stack.Sent().empty()) << "bit " << bit << " of the header flipped\n"
                                          << stack.SentText();
    }
}

TEST(Ipv4Test, Rfc1122_WrongChecksumValuesAreDiscarded)
{
    const Bytes good = Ipv4Datagram(EchoRequest());
    const uint16_t right = Be16(good, 10);
    for (int delta : {1, 2, 255, 256, 0x8000})
    {
        Ipv4Fields ip = EchoRequest();
        ip.checksum = static_cast<uint16_t>(right + delta);
        EXPECT_TRUE(SentAfter(ip).empty()) << "checksum off by " << delta;
    }
}

TEST(Ipv4Test, Rfc791_OptionsAreSkippedUsingTheHeaderLength)
{
    // 01 is no operation and 00 is the end of the list
    const std::vector<Bytes> options = {{1, 1, 1, 0}, {1, 1, 1, 1, 1, 1, 1, 0}, {0, 0, 0, 0}};
    for (const Bytes& option : options)
    {
        Ipv4Fields ip = EchoRequest();
        ip.options = option;
        std::vector<Bytes> sent = SentAfter(ip);
        ASSERT_EQ(1u, sent.size()) << option.size() << " octets of options";

        // The reply has no options, and carries the data of the request
        SentIp reply = ParseIp(sent[0]);
        EXPECT_EQ(0x45, reply.versionAndHeaderLength);
        EXPECT_EQ(EchoData, Slice(sent[0], 14 + 20 + 8, 8));
    }
}

TEST(Ipv4Test, Rfc791_FirstFragmentIsNotTakenForACompleteDatagram)
{
    // The stack does not reassemble fragments. Handing the first one on as if it were the whole
    // datagram would be worse than dropping it.
    Ipv4Fields ip = EchoRequest();
    ip.flagsAndFragmentOffset = 0x2000; // more fragments, offset 0
    EXPECT_TRUE(SentAfter(ip).empty());
}

TEST(Ipv4Test, Rfc791_LaterFragmentIsNotTakenForACompleteDatagram)
{
    for (uint16_t flagsAndOffset : {0x0001, 0x00b9, 0x2001, 0x1fff})
    {
        Ipv4Fields ip = EchoRequest();
        ip.flagsAndFragmentOffset = flagsAndOffset;
        EXPECT_TRUE(SentAfter(ip).empty())
            << "fragment offset field " << std::hex << flagsAndOffset;
    }
}

TEST(Ipv4Test, Rfc791_DontFragmentDoesNotStopADatagramBeingReceived)
{
    Ipv4Fields ip = EchoRequest();
    ip.flagsAndFragmentOffset = 0x4000;
    EXPECT_EQ(1u, SentAfter(ip).size());
}

TEST(Ipv4Test, Rfc791_DatagramOf576OctetsIsAccepted)
{
    // 20 octets of header, 8 of ICMP and 548 of data. The data comes back whole.
    const Bytes data(548, 0x5a);
    Ipv4Fields ip = EchoRequest();
    ip.payload = IcmpEcho(8, 1, 1, data);
    ASSERT_EQ(576u, Ipv4Datagram(ip).size());

    std::vector<Bytes> sent = SentAfter(ip);
    ASSERT_EQ(1u, sent.size());
    EXPECT_EQ(576, ParseIp(sent[0]).totalLength);
    EXPECT_EQ(data, Slice(sent[0], 14 + 20 + 8, 548));
}

TEST(Ipv4Test, Rfc1122_DatagramWithASmallTtlIsStillAccepted)
{
    // A host must not discard a datagram just because the TTL is less than 2
    for (int ttl : {0, 1, 2, 64, 255})
    {
        Ipv4Fields ip = EchoRequest();
        ip.ttl = static_cast<uint8_t>(ttl);
        EXPECT_EQ(1u, SentAfter(ip).size()) << "TTL " << ttl;
    }
}

TEST(Ipv4Test, Rfc2474_DsAndEcnBitsAreIgnored)
{
    for (int tos : {0x00, 0x01, 0x02, 0x03, 0x04, 0x20, 0xb8, 0xfc, 0xff})
    {
        Ipv4Fields ip = EchoRequest();
        ip.typeOfService = static_cast<uint8_t>(tos);
        EXPECT_EQ(1u, SentAfter(ip).size()) << "type of service " << tos;
    }
}

// ---------------------------------------------------------------------------------------------
// Addresses
// ---------------------------------------------------------------------------------------------

TEST(Ipv4Test, Rfc1122_DatagramThatIsNotForTheStackIsDiscarded)
{
    // The stack is a host, so it does not forward
    for (const Bytes& destination :
         {OtherIp, Bytes({10, 1, 2, 3}), Bytes({127, 0, 0, 1}), Bytes({192, 168, 2, 50})})
    {
        Ipv4Fields ip = EchoRequest();
        ip.destination = destination;
        EXPECT_TRUE(SentAfter(ip).empty()) << "to " << Hex(destination);
    }
}

TEST(Ipv4Test, Rfc1122_DatagramWithAnInvalidSourceAddressIsDiscarded)
{
    // Broadcast, loopback and multicast addresses are not the address of one host
    const std::vector<Bytes> sources = {Bytes({255, 255, 255, 255}),
                                        SubnetBroadcastIp,
                                        Bytes({127, 0, 0, 1}),
                                        Bytes({127, 255, 255, 254}),
                                        Bytes({224, 0, 0, 1}),
                                        Bytes({239, 255, 255, 250})};
    for (const Bytes& source : sources)
    {
        Ipv4Fields ip = EchoRequest();
        ip.source = source;
        EXPECT_TRUE(SentAfter(ip).empty()) << "from " << Hex(source);
    }
}

TEST(Ipv4Test, Rfc1122_EchoReplyToABroadcastRequestComesFromTheStacksOwnAddress)
{
    // A host may answer an echo request sent to a broadcast address or not, but if it does the
    // reply is from its own address and not from the broadcast address
    for (const Bytes& destination : {Bytes({255, 255, 255, 255}), SubnetBroadcastIp})
    {
        Ipv4Fields ip = EchoRequest();
        ip.destination = destination;
        for (const Bytes& frame : SentAfter(ip, BroadcastMac))
        {
            SentIp reply = ParseIp(frame);
            EXPECT_EQ(StackIp, reply.source) << "request to " << Hex(destination);
            EXPECT_EQ(PeerIp, reply.destination);
        }
    }
}

TEST(Ipv4Test, Rfc1122_UnicastDatagramInALinkLayerBroadcastFrameIsDiscarded)
{
    EXPECT_TRUE(SentAfter(EchoRequest(), BroadcastMac).empty());
}

// ---------------------------------------------------------------------------------------------
// Where a datagram goes
// ---------------------------------------------------------------------------------------------

TEST(Ipv4Test, Rfc950_ReplyToAHostOnTheSubnetGoesStraightToIt)
{
    std::vector<Bytes> sent = SentAfter(EchoRequest());
    ASSERT_EQ(1u, sent.size());
    EXPECT_EQ(PeerMac, Slice(sent[0], 0, 6));
}

TEST(Ipv4Test, Rfc950_ReplyToAHostOffTheSubnetGoesToTheGateway)
{
    StackUnderTest stack;
    Ipv4Fields ip = EchoRequest();
    ip.source = OffSubnetIp;
    stack.Receive(Frame(ip, StackMac, GatewayMac)); // it arrives by way of the gateway

    // The stack does not know the gateway's Ethernet address, so it asks for it
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    EXPECT_EQ(EtherTypeArp, Be16(stack.Sent()[0], 12));
    EXPECT_EQ(1, Be16(stack.Sent()[0], 14 + 6)) << "an ARP request";
    EXPECT_EQ(GatewayIp, Slice(stack.Sent()[0], 14 + 24, 4)) << "for the gateway, not the host";
    stack.ClearSent();

    stack.Receive(EthernetFrame(StackMac,
                                GatewayMac,
                                EtherTypeArp,
                                ArpPacket(ArpReply(GatewayMac, GatewayIp, StackMac, StackIp))));
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    EXPECT_EQ(GatewayMac, Slice(stack.Sent()[0], 0, 6)) << "Ethernet destination";
    EXPECT_EQ(OffSubnetIp, ParseIp(stack.Sent()[0]).destination) << "IP destination";
}

TEST(Ipv4Test, Rfc950_TheSubnetMaskDecidesWhatIsOnTheLocalNetwork)
{
    struct Case
    {
        Bytes mask;
        Bytes host;
        bool local;
    };
    const std::vector<Case> cases = {
        {{255, 255, 255, 0}, {192, 168, 1, 20}, true},
        {{255, 255, 255, 0}, {192, 168, 77, 5}, false},
        {{255, 255, 0, 0}, {192, 168, 77, 5}, true},
        {{255, 255, 255, 128}, {192, 168, 1, 100}, true},
        {{255, 255, 255, 128}, {192, 168, 1, 200}, false},
        {{255, 255, 255, 192}, {192, 168, 1, 100}, false},
        {{255, 255, 255, 192}, {192, 168, 1, 60}, true},
    };
    for (const Case& c : cases)
    {
        StackUnderTest::Network network;
        network.mask = c.mask;
        StackUnderTest stack(network);
        stack.stack().ARP.Protocol2Hardware(c.host.data());
        ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();

        // A host on the local network is asked for directly, any other is reached by the gateway
        EXPECT_EQ(c.local ? c.host : GatewayIp, Slice(stack.Sent()[0], 14 + 24, 4))
            << "host " << Hex(c.host) << "mask " << Hex(c.mask);
    }
}

// ---------------------------------------------------------------------------------------------
// Broadcasts
// ---------------------------------------------------------------------------------------------

TEST(Ipv4Test, Rfc919_LimitedBroadcastGoesToTheBroadcastEthernetAddress)
{
    StackUnderTest stack;
    SendDatagram(stack, 253, Bytes({255, 255, 255, 255}), Bytes(20, 0x11));
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    EXPECT_EQ(BroadcastMac, Slice(stack.Sent()[0], 0, 6));
    EXPECT_EQ(EtherTypeIp, Be16(stack.Sent()[0], 12));
}

TEST(Ipv4Test, Rfc922_SubnetBroadcastGoesToTheBroadcastEthernetAddress)
{
    // Not asked for with ARP, because nobody has the broadcast address
    StackUnderTest stack;
    SendDatagram(stack, 253, SubnetBroadcastIp, Bytes(20, 0x11));
    ASSERT_EQ(1u, stack.Sent().size()) << stack.SentText();
    EXPECT_EQ(BroadcastMac, Slice(stack.Sent()[0], 0, 6));
    EXPECT_EQ(EtherTypeIp, Be16(stack.Sent()[0], 12));
}

TEST(Ipv4Test, Rfc922_TheSubnetBroadcastAddressFollowsTheSubnetMask)
{
    struct Case
    {
        Bytes mask;
        Bytes broadcast;
        Bytes notBroadcast;
    };
    const std::vector<Case> cases = {
        {{255, 255, 255, 0}, {192, 168, 1, 255}, {192, 168, 1, 127}},
        {{255, 255, 0, 0}, {192, 168, 255, 255}, {192, 168, 1, 255}},
        {{255, 255, 255, 128}, {192, 168, 1, 127}, {192, 168, 1, 255}},
        {{255, 255, 255, 192}, {192, 168, 1, 63}, {192, 168, 1, 127}},
    };
    for (const Case& c : cases)
    {
        StackUnderTest::Network network;
        network.mask = c.mask;

        StackUnderTest broadcast(network);
        SendDatagram(broadcast, 253, c.broadcast, Bytes(20, 0x11));
        ASSERT_EQ(1u, broadcast.Sent().size()) << "mask " << Hex(c.mask) << broadcast.SentText();
        EXPECT_EQ(BroadcastMac, Slice(broadcast.Sent()[0], 0, 6)) << "mask " << Hex(c.mask);
        // It is the datagram that goes out, not an ARP request for the broadcast address, which
        // would go to the broadcast Ethernet address too
        EXPECT_EQ(EtherTypeIp, Be16(broadcast.Sent()[0], 12)) << "mask " << Hex(c.mask);

        // An address that is an ordinary host on this subnet is asked for with ARP
        StackUnderTest ordinary(network);
        SendDatagram(ordinary, 253, c.notBroadcast, Bytes(20, 0x11));
        ASSERT_EQ(1u, ordinary.Sent().size()) << ordinary.SentText();
        EXPECT_EQ(EtherTypeArp, Be16(ordinary.Sent()[0], 12))
            << Hex(c.notBroadcast) << "with mask " << Hex(c.mask) << "is not a broadcast";
    }
}

// ---------------------------------------------------------------------------------------------
// The datagrams the stack sends
// ---------------------------------------------------------------------------------------------

TEST(Ipv4Test, Rfc791_SentDatagramHasAWellFormedHeader)
{
    std::vector<Bytes> sent = SentAfter(EchoRequest());
    ASSERT_EQ(1u, sent.size());
    SentIp ip = ParseIp(sent[0]);

    EXPECT_EQ(0x45, ip.versionAndHeaderLength) << "version 4, 5 words of header";
    EXPECT_EQ(0, ip.typeOfService);
    EXPECT_EQ(20 + 8 + 8, ip.totalLength) << "header, ICMP header and the 8 octets of data";
    EXPECT_EQ(0, ip.flagsAndFragmentOffset & 0x3fff) << "not fragmented";
    EXPECT_EQ(0, ip.flagsAndFragmentOffset & 0x8000) << "the reserved flag is zero";
    EXPECT_EQ(1, ip.protocol) << "ICMP";
    EXPECT_EQ(StackIp, ip.source);
    EXPECT_EQ(PeerIp, ip.destination);
}

TEST(Ipv4Test, Rfc791_HeaderChecksumOfASentDatagramIsRight)
{
    std::vector<Bytes> sent = SentAfter(EchoRequest());
    ASSERT_EQ(1u, sent.size());
    EXPECT_EQ(0, InternetChecksum(ParseIp(sent[0]).header))
        << "the checksum over a header that is right is zero";
}

TEST(Ipv4Test, Rfc1122_TtlOfASentDatagramIsNotZero)
{
    std::vector<Bytes> sent = SentAfter(EchoRequest());
    ASSERT_EQ(1u, sent.size());
    EXPECT_GE(ParseIp(sent[0]).ttl, 1);
}

TEST(Ipv4Test, Rfc1122_DefaultTtlIs64)
{
    // RFC 1122 leaves the value to the Assigned Numbers registry, which says 64
    std::vector<Bytes> sent = SentAfter(EchoRequest());
    ASSERT_EQ(1u, sent.size());
    EXPECT_EQ(64, ParseIp(sent[0]).ttl);
}

TEST(Ipv4Test, Rfc791_IdentificationOfSuccessiveDatagramsDiffers)
{
    StackUnderTest stack;
    TeachArp(stack, PeerMac, PeerIp);
    stack.Receive(Frame(EchoRequest()));
    stack.Receive(Frame(EchoRequest()));
    stack.Receive(Frame(EchoRequest()));
    ASSERT_EQ(3u, stack.Sent().size()) << stack.SentText();
    uint16_t a = ParseIp(stack.Sent()[0]).id;
    uint16_t b = ParseIp(stack.Sent()[1]).id;
    uint16_t c = ParseIp(stack.Sent()[2]).id;
    EXPECT_NE(a, b);
    EXPECT_NE(b, c);
    EXPECT_NE(a, c);
}
