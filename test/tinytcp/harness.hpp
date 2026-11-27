#pragma once

// Test harness for the protocol layers. It builds a DefaultStack, feeds it frames as if they came
// from the wire and records every frame the stack transmits. The frame builders here do not use
// any tinytcp code, so the tests do not depend on the code they are checking.

#include <memory>
#include <new>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "tinytcp/Config.hpp"
#include "tinytcp/DefaultStack.hpp"

namespace tt = tinytcp;

namespace tt_test
{

typedef std::vector<uint8_t> Bytes;

// The stack under test and the hosts around it
const Bytes StackMac = {0x02, 0x00, 0x5e, 0x00, 0x00, 0x01};
const Bytes StackIp = {192, 168, 1, 50};
const Bytes PeerMac = {0x02, 0x00, 0x5e, 0x00, 0x00, 0x20};
const Bytes PeerIp = {192, 168, 1, 20};
const Bytes OtherMac = {0x02, 0x00, 0x5e, 0x00, 0x00, 0x30};
const Bytes OtherIp = {192, 168, 1, 30};
const Bytes SubnetMask = {255, 255, 255, 0};
const Bytes GatewayIp = {192, 168, 1, 1};
const Bytes SubnetBroadcastIp = {192, 168, 1, 255};
const Bytes BroadcastMac = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
const Bytes ZeroMac = {0, 0, 0, 0, 0, 0};
const Bytes ZeroIp = {0, 0, 0, 0};

const uint16_t EtherTypeIp = 0x0800;
const uint16_t EtherTypeArp = 0x0806;

// ---------------------------------------------------------------------------------------------
// Building and reading frames
// ---------------------------------------------------------------------------------------------

inline void Append(Bytes& to, const Bytes& more)
{
    to.insert(to.end(), more.begin(), more.end());
}

inline void Append16(Bytes& to, uint16_t value)
{
    to.push_back(static_cast<uint8_t>(value >> 8));
    to.push_back(static_cast<uint8_t>(value & 0xff));
}

inline uint16_t Be16(const Bytes& from, size_t offset)
{
    return static_cast<uint16_t>((from.at(offset) << 8) | from.at(offset + 1));
}

inline Bytes Slice(const Bytes& from, size_t offset, size_t length)
{
    return Bytes(from.begin() + offset, from.begin() + offset + length);
}

inline std::string Hex(const Bytes& bytes)
{
    std::string text;
    char digits[4];
    for (uint8_t byte : bytes)
    {
        snprintf(digits, sizeof(digits), "%02x ", byte);
        text += digits;
    }
    return text;
}

// An Ethernet frame (RFC 894): destination, source, type, data. A frame shorter than padTo is
// padded with zero octets, as the sender is required to do. The checksum at the end of a real
// frame is added and removed by the network hardware, so it is not here.
inline Bytes EthernetFrame(const Bytes& destination,
                           const Bytes& source,
                           uint16_t type,
                           const Bytes& data,
                           size_t padTo = 60)
{
    Bytes frame;
    Append(frame, destination);
    Append(frame, source);
    Append16(frame, type);
    Append(frame, data);
    while (frame.size() < padTo)
    {
        frame.push_back(0);
    }
    return frame;
}

// The fields of an ARP packet (RFC 826). The address lengths are not taken from the addresses so a
// test can build a packet whose lengths do not match.
struct ArpFields
{
    uint16_t hardwareType = 1;
    uint16_t protocolType = 0x0800;
    uint8_t hardwareSize = 6;
    uint8_t protocolSize = 4;
    uint16_t opcode = 1;
    Bytes senderMac;
    Bytes senderIp;
    Bytes targetMac;
    Bytes targetIp;
};

inline Bytes ArpPacket(const ArpFields& arp)
{
    Bytes packet;
    Append16(packet, arp.hardwareType);
    Append16(packet, arp.protocolType);
    packet.push_back(arp.hardwareSize);
    packet.push_back(arp.protocolSize);
    Append16(packet, arp.opcode);
    Append(packet, arp.senderMac);
    Append(packet, arp.senderIp);
    Append(packet, arp.targetMac);
    Append(packet, arp.targetIp);
    return packet;
}

inline ArpFields ArpRequest(const Bytes& senderMac, const Bytes& senderIp, const Bytes& targetIp)
{
    ArpFields arp;
    arp.opcode = 1;
    arp.senderMac = senderMac;
    arp.senderIp = senderIp;
    arp.targetMac = ZeroMac;
    arp.targetIp = targetIp;
    return arp;
}

inline ArpFields ArpReply(const Bytes& senderMac,
                          const Bytes& senderIp,
                          const Bytes& targetMac,
                          const Bytes& targetIp)
{
    ArpFields arp;
    arp.opcode = 2;
    arp.senderMac = senderMac;
    arp.senderIp = senderIp;
    arp.targetMac = targetMac;
    arp.targetIp = targetIp;
    return arp;
}

// The Internet checksum of RFC 1071, written independently of the code under test.
inline uint16_t InternetChecksum(const Bytes& data)
{
    uint32_t sum = 0;
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

// An IPv4 datagram (RFC 791) with a correct header checksum and no options
inline Bytes Ipv4Packet(const Bytes& source,
                        const Bytes& destination,
                        uint8_t protocol,
                        const Bytes& payload,
                        uint8_t ttl = 64,
                        uint16_t id = 1)
{
    Bytes header;
    header.push_back(0x45); // version 4, header length 5 words
    header.push_back(0);    // type of service
    Append16(header, static_cast<uint16_t>(20 + payload.size()));
    Append16(header, id);
    Append16(header, 0); // flags and fragment offset
    header.push_back(ttl);
    header.push_back(protocol);
    Append16(header, 0); // header checksum, filled in below
    Append(header, source);
    Append(header, destination);
    uint16_t checksum = InternetChecksum(header);
    header[10] = static_cast<uint8_t>(checksum >> 8);
    header[11] = static_cast<uint8_t>(checksum & 0xff);

    Bytes packet = header;
    Append(packet, payload);
    return packet;
}

// Every field of an IPv4 header (RFC 791), so a test can change one thing and leave the rest right.
// A field left at its default is filled in correctly: the header length from the options, the
// total length from the payload and the checksum from the rest of the header.
struct Ipv4Fields
{
    int versionAndHeaderLength = -1; // -1 means 4 and the length that fits the options
    uint8_t typeOfService = 0;
    int totalLength = -1; // -1 means the header and the payload
    uint16_t id = 1;
    uint16_t flagsAndFragmentOffset = 0;
    uint8_t ttl = 64;
    uint8_t protocol = 1;
    Bytes source = PeerIp;
    Bytes destination = StackIp;
    Bytes options;     // a multiple of 4 octets
    int checksum = -1; // -1 means the correct checksum
    Bytes payload;
};

inline Bytes Ipv4Datagram(const Ipv4Fields& ip)
{
    Bytes header;
    size_t headerLength = 20 + ip.options.size();
    header.push_back(ip.versionAndHeaderLength >= 0
                         ? static_cast<uint8_t>(ip.versionAndHeaderLength)
                         : static_cast<uint8_t>(0x40 | (headerLength / 4)));
    header.push_back(ip.typeOfService);
    Append16(header,
             ip.totalLength >= 0 ? static_cast<uint16_t>(ip.totalLength)
                                 : static_cast<uint16_t>(headerLength + ip.payload.size()));
    Append16(header, ip.id);
    Append16(header, ip.flagsAndFragmentOffset);
    header.push_back(ip.ttl);
    header.push_back(ip.protocol);
    Append16(header, 0); // checksum, filled in below
    Append(header, ip.source);
    Append(header, ip.destination);
    Append(header, ip.options);
    uint16_t checksum =
        ip.checksum >= 0 ? static_cast<uint16_t>(ip.checksum) : InternetChecksum(header);
    header[10] = static_cast<uint8_t>(checksum >> 8);
    header[11] = static_cast<uint8_t>(checksum & 0xff);

    Bytes datagram = header;
    Append(datagram, ip.payload);
    return datagram;
}

// An ICMP echo request (type 8) or reply (type 0) with a correct checksum (RFC 792)
inline Bytes IcmpEcho(uint8_t type, uint16_t identifier, uint16_t sequence, const Bytes& data)
{
    Bytes message;
    message.push_back(type);
    message.push_back(0); // code
    Append16(message, 0); // checksum, filled in below
    Append16(message, identifier);
    Append16(message, sequence);
    Append(message, data);
    uint16_t checksum = InternetChecksum(message);
    message[2] = static_cast<uint8_t>(checksum >> 8);
    message[3] = static_cast<uint8_t>(checksum & 0xff);
    return message;
}

// ---------------------------------------------------------------------------------------------
// The stack under test
// ---------------------------------------------------------------------------------------------

class StackUnderTest;

inline StackUnderTest*& CurrentStack()
{
    static StackUnderTest* current = nullptr;
    return current;
}

class StackUnderTest
{
public:
    enum Setup
    {
        MacAndAddress, // the stack has its Ethernet address and an IPv4 address
        MacOnly        // as before DHCP has given it an address
    };

    // The IPv4 configuration of the stack when it has one
    struct Network
    {
        Bytes address = StackIp;
        Bytes mask = SubnetMask;
        Bytes gateway = GatewayIp;
    };

    explicit StackUnderTest(Setup setup = MacAndAddress)
        : StackUnderTest(setup, Network())
    {
    }

    explicit StackUnderTest(const Network& network)
        : StackUnderTest(MacAndAddress, network)
    {
    }

    StackUnderTest(Setup setup, const Network& network)
    {
        // Build the stack in memory full of 0xAA so that anything the stack reads without having
        // written it first is a recognisable pattern, not an accidental zero
        void* memory = ::operator new(sizeof(tt::DefaultStack));
        memset(memory, 0xAA, sizeof(tt::DefaultStack));
        m_stack = new (memory) tt::DefaultStack();

        CurrentStack() = this;
        m_stack->RegisterDataTransmitHandler(CaptureFrame);

        Bytes mac = StackMac;
        m_stack->SetMACAddress(mac.data());
        if (setup == MacAndAddress)
        {
            tt::ProtocolIPv4::AddressInfo info;
            memset(&info, 0, sizeof(info));
            info.DataValid = true;
            memcpy(info.Address, network.address.data(), 4);
            memcpy(info.SubnetMask, network.mask.data(), 4);
            memcpy(info.Gateway, network.gateway.data(), 4);
            for (int i = 0; i < 4; i++)
            {
                info.BroadcastAddress[i] = network.address[i] | ~network.mask[i];
            }
            m_stack->IP.SetAddressInfo(info);
        }
    }

    ~StackUnderTest()
    {
        CurrentStack() = nullptr;
        m_stack->~DefaultStack();
        ::operator delete(m_stack);
    }

    StackUnderTest(const StackUnderTest&) = delete;
    StackUnderTest& operator=(const StackUnderTest&) = delete;

    tt::DefaultStack& stack() { return *m_stack; }

    // Hand a frame to the stack as if it had arrived from the wire
    void Receive(const Bytes& frame)
    {
        Bytes copy = frame;
        m_stack->ProcessRx(copy.data(), copy.size());
    }

    // The frames the stack has transmitted since the last ClearSent()
    const std::vector<Bytes>& Sent() const { return m_sent; }

    void ClearSent() { m_sent.clear(); }

    std::string SentText() const
    {
        std::string text;
        for (const Bytes& frame : m_sent)
        {
            text += "  " + Hex(frame) + "\n";
        }
        return text;
    }

private:
    static void CaptureFrame(void* data, size_t length)
    {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        CurrentStack()->m_sent.push_back(Bytes(bytes, bytes + length));
    }

    tt::DefaultStack* m_stack;
    std::vector<Bytes> m_sent;
};

// Hand the stack a datagram to send, as a transport layer does, and let it go out
inline void SendDatagram(StackUnderTest& stack,
                         uint8_t protocol,
                         const Bytes& destination,
                         const Bytes& payload)
{
    tt::DataBuffer* buffer = stack.stack().IP.GetTxBuffer(&stack.stack().MAC);
    memcpy(buffer->Packet, payload.data(), payload.size());
    buffer->Length = static_cast<uint16_t>(payload.size());
    stack.stack().IP.Transmit(buffer, protocol, destination.data(), StackIp.data());
}

// Make the stack learn that peerIp is at peerMac, using an ARP reply addressed to the stack
inline void TeachArp(StackUnderTest& stack, const Bytes& peerMac, const Bytes& peerIp)
{
    stack.Receive(EthernetFrame(
        StackMac, peerMac, EtherTypeArp, ArpPacket(ArpReply(peerMac, peerIp, StackMac, StackIp))));
    stack.ClearSent();
}

// The stack has RX_BUFFER_COUNT receive buffers and uses them in turn. After this the next frame
// lands in a buffer that still holds the bytes of an earlier frame, which is how a frame that is
// shorter than it claims to be gets noticed.
inline void UseEveryReceiveBuffer(StackUnderTest& stack)
{
    for (int i = 0; i < RX_BUFFER_COUNT; i++)
    {
        stack.Receive(EthernetFrame(
            StackMac, PeerMac, EtherTypeArp, ArpPacket(ArpRequest(PeerMac, PeerIp, StackIp))));
    }
    stack.ClearSent();
}

} // namespace tt_test
