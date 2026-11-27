//----------------------------------------------------------------------------
// Copyright(c) 2015, Robert Kimball
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//----------------------------------------------------------------------------

#include <stdio.h>

#include "DataBuffer.hpp"
#include "FCS.hpp"
#include "InterfaceMAC.hpp"
#include "ProtocolARP.hpp"
#include "ProtocolICMP.hpp"
#include "ProtocolIPv4.hpp"
#include "ProtocolTCP.hpp"
#include "ProtocolUDP.hpp"
#include "Utility.hpp"

namespace tinytcp
{

// Version - 4 bits
// Header Length - 4 bits
// Type of Service - 8 bits
// TotalLength - 16 bits
// Identification - 16 bits
// Flags - 3 bits
// FragmentOffset - 13 bits
// TTL - 8 bits
// Protocol - 8 bits
// HeaderChecksum - 16 bits

ProtocolIPv4::ProtocolIPv4(
    InterfaceMAC& mac, ProtocolARP& arp, ProtocolICMP& icmp, ProtocolTCP& tcp, ProtocolUDP& udp)
    : PacketID(0)
    , UnresolvedQueue("IP", TX_BUFFER_COUNT, TxBuffer)
    , Address()
    , MAC(mac)
    , ARP(arp)
    , ICMP(icmp)
    , TCP(tcp)
    , UDP(udp)
{
    Address.DataValid = false;
}

bool ProtocolIPv4::IsBroadcastAddress(const uint8_t* addr)
{
    bool allOnes = true;
    for (size_t i = 0; i < AddressSize(); i++)
    {
        if (addr[i] != 0xFF)
        {
            allOnes = false;
        }
    }
    if (allOnes)
    {
        return true; // the limited broadcast address, 255.255.255.255
    }

    if (!Address.DataValid)
    {
        return false;
    }

    // A mask with no host bits leaves no room for a broadcast address
    bool hostBits = false;
    for (size_t i = 0; i < AddressSize(); i++)
    {
        if (Address.SubnetMask[i] != 0xFF)
        {
            hostBits = true;
        }
    }
    if (!hostBits)
    {
        return false;
    }

    // On our subnet, with every host bit set
    for (size_t i = 0; i < AddressSize(); i++)
    {
        if ((addr[i] & Address.SubnetMask[i]) != (Address.Address[i] & Address.SubnetMask[i]) ||
            (addr[i] | Address.SubnetMask[i]) != 0xFF)
        {
            return false;
        }
    }
    return true;
}

bool ProtocolIPv4::IsLocal(const uint8_t* addr)
{
    return IsBroadcastAddress(addr) ||
           (Address.DataValid && AddressCompare(addr, GetUnicastAddress(), AddressSize()));
}

// RFC 1122 section 3.2.1.3: a broadcast address, a loopback address or a multicast address is not
// the address of a host, so a datagram cannot come from one. 0.0.0.0 can: it is how a host that has
// no address yet, one that is asking for one with DHCP, says so.
static bool IsValidSource(ProtocolIPv4& ip, const uint8_t* addr)
{
    return !ip.IsBroadcastAddress(addr) && addr[0] != 127 && (addr[0] & 0xF0) != 0xE0;
}

void ProtocolIPv4::ProcessRx(DataBuffer* buffer)
{
    uint8_t headerLength;
    uint8_t protocol;
    uint8_t* sourceIP;
    uint8_t* targetIP;
    uint8_t* packet = buffer->Packet;
    uint16_t length = buffer->Length;
    uint16_t totalLength;

    // RFC 791: the fixed part of the header is 20 octets
    if (length < header_size())
    {
        return;
    }

    // RFC 1122 section 3.2.1.1: a datagram whose version is not 4 is silently discarded
    if ((packet[0] >> 4) != 4)
    {
        return;
    }

    // RFC 791: the header length is in 32 bit words and is at least 5. The total length counts the
    // header and the data, so it cannot be less than the header, and it cannot be more than what
    // arrived. Anything after the total length is padding.
    headerLength = (packet[0] & 0x0F) * 4;
    totalLength = Unpack16(packet, 2);
    if (headerLength < header_size() || totalLength < headerLength || totalLength > length)
    {
        return;
    }

    // RFC 1122 section 3.2.1.2: the header checksum is verified, and a datagram with a bad one is
    // silently discarded. A header that has its checksum in it adds up to zero.
    if (FCS::Checksum(packet, headerLength) != 0)
    {
        return;
    }

    protocol = packet[9];
    sourceIP = &packet[12];
    targetIP = &packet[16];

    // RFC 1122 section 3.2.1.3: a datagram that is not for this host is silently discarded, and so
    // is one with a source address that cannot be a host
    if (!IsLocal(targetIP) || !IsValidSource(*this, sourceIP))
    {
        return;
    }

    // RFC 1122 section 3.3.6: a datagram that came in a link layer broadcast, and is not for an IP
    // broadcast address, should be silently discarded
    if (buffer->LinkBroadcast && !IsBroadcastAddress(targetIP))
    {
        return;
    }

    // RFC 791 and RFC 1122 section 3.3.2 say a host must reassemble fragments. This stack does not,
    // and a fragment must not be passed on as if it were a whole datagram, so fragments are
    // dropped. The flags are the top 3 bits of the field. More fragments is 0x2000, and the rest is
    // the offset.
    if ((Unpack16(packet, 6) & 0x3FFF) != 0)
    {
        return;
    }

    buffer->Packet += headerLength;
    buffer->Length = totalLength - headerLength;

    switch (protocol)
    {
    case 0x01: // ICMP
        ICMP.ProcessRx(buffer, sourceIP, targetIP);
        break;
    case 0x02: // IGMP
        break;
    case 0x06: // TCP
        TCP.ProcessRx(buffer, sourceIP, targetIP);
        break;
    case 0x11: // UDP
        UDP.ProcessRx(buffer, sourceIP, targetIP);
        break;
    default:
        // RFC 1122 section 3.2.2.1: a protocol that is not supported is answered with protocol
        // unreachable, which is code 2
        ICMP.SendDestinationUnreachable(2, buffer, packet, totalLength);
        break;
    }
}

DataBuffer* ProtocolIPv4::GetTxBuffer(InterfaceMAC* mac)
{
    DataBuffer* buffer;

    buffer = mac->GetTxBuffer();
    if (buffer != nullptr)
    {
        buffer->Packet += header_size();
        buffer->Remainder -= header_size();
    }

    return buffer;
}

void ProtocolIPv4::Transmit(DataBuffer* buffer,
                            uint8_t protocol,
                            const uint8_t* targetIP,
                            const uint8_t* sourceIP)
{
    uint16_t checksum;
    const uint8_t* targetMAC;
    uint8_t* packet;

    buffer->Packet -= header_size();
    buffer->Length += header_size();
    packet = buffer->Packet;

    packet[0] = 0x45; // Version and HeaderSize
    packet[1] = 0;    // ToS
    Pack16(packet, 2, buffer->Length);

    PacketID++;
    Pack16(packet, 4, PacketID);
    packet[6] = 0; // Flags & FragmentOffset
    packet[7] = 0; // rest of FragmentOffset

    packet[8] = IPV4_DEFAULT_TTL; // TTL
    packet[9] = protocol;
    Pack16(packet, 10, 0); // checksum placeholder
    PackBytes(packet, 12, sourceIP, 4);
    PackBytes(packet, 16, targetIP, 4);

    checksum = FCS::Checksum(packet, 20);
    Pack16(packet, 10, checksum);

    targetMAC = ARP.Protocol2Hardware(targetIP);
    if (targetMAC != nullptr)
    {
        MAC.Transmit(buffer, targetMAC, 0x0800);
    }
    else
    {
        // Could not find MAC address, ARP for it
        UnresolvedQueue.Put(buffer);
    }
}

void ProtocolIPv4::Retransmit(DataBuffer* buffer)
{
    MAC.Retransmit(buffer);
}

void ProtocolIPv4::Retry()
{
    int count;
    DataBuffer* buffer;
    const uint8_t* targetMAC;

    count = UnresolvedQueue.GetCount();
    for (int i = 0; i < count; i++)
    {
        buffer = (DataBuffer*)UnresolvedQueue.Get();

        targetMAC = ARP.Protocol2Hardware(&buffer->Packet[16]);
        if (targetMAC != nullptr)
        {
            MAC.Transmit(buffer, targetMAC, 0x0800);
        }
        else
        {
            printf("Still could not find MAC for IP\n");
            UnresolvedQueue.Put(buffer);
        }
    }
}

void ProtocolIPv4::FreeTxBuffer(DataBuffer* buffer)
{
    MAC.FreeTxBuffer(buffer);
}

void ProtocolIPv4::FreeRxBuffer(DataBuffer* buffer)
{
    MAC.FreeRxBuffer(buffer);
}

void ProtocolIPv4::dump_info(os::TextOutput output, void* context) const
{
    os::Print(output, context, "IPv4 Configuration\n");
    os::Print(output, context, "   Address:            %s\n", ipv4toa(Address.Address));
    os::Print(output, context, "   Subnet Mask:        %s\n", ipv4toa(Address.SubnetMask));
    os::Print(output, context, "   Gateway:            %s\n", ipv4toa(Address.Gateway));
    os::Print(output, context, "   Domain Name Server: %s\n", ipv4toa(Address.DomainNameServer));
    os::Print(output, context, "   Broadcast Address:  %s\n", ipv4toa(Address.BroadcastAddress));
    os::Print(output,
              context,
              "   Address Lease Time: %lu seconds\n",
              (unsigned long)Address.IpAddressLeaseTime);
    os::Print(
        output, context, "   RenewTime:          %lu seconds\n", (unsigned long)Address.RenewTime);
    os::Print(
        output, context, "   RebindTime:         %lu seconds\n", (unsigned long)Address.RebindTime);
}

size_t ProtocolIPv4::AddressSize()
{
    return ADDRESS_SIZE;
}

const uint8_t* ProtocolIPv4::GetUnicastAddress()
{
    return Address.Address;
}

const uint8_t* ProtocolIPv4::GetBroadcastAddress()
{
    return Address.BroadcastAddress;
}

const uint8_t* ProtocolIPv4::GetGatewayAddress()
{
    return Address.Gateway;
}

const uint8_t* ProtocolIPv4::GetSubnetMask()
{
    return Address.SubnetMask;
}

void ProtocolIPv4::SetAddressInfo(const AddressInfo& info)
{
    Address = info;
}

} // namespace tinytcp
