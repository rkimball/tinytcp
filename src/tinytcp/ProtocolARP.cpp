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

#include "Config.hpp"
#include "DataBuffer.hpp"
#include "InterfaceMAC.hpp"
#include "ProtocolARP.hpp"
#include "ProtocolIPv4.hpp"
#include "Utility.hpp"

namespace tinytcp
{

// HardwareType - 2 bytes
// ProtocolType - 2 bytes
// HardwareSize - 1 byte, size int bytes of HardwareAddress fields
// IPv4AddressSize - 1 byte, size int bytes of ProtocolAddress fields
// Op - 2 bytes
// SenderHardwareAddress - HardwareSize bytes
// SenderProtocolAddress - IPv4AddressSize bytes
// TargetHardwareAddress - HardwareSize bytes
// TargetProtocolAddress - IPv4AddressSize bytes

ARPCacheEntry::ARPCacheEntry()
    : Age(0)
{
}

ProtocolARP::ProtocolARP(InterfaceMAC& mac, ProtocolIPv4& ip)
    : MAC(mac)
    , IP(ip)
{
}

void ProtocolARP::Initialize() {}

void ProtocolARP::ProcessRx(const DataBuffer* buffer)
{
    uint8_t* packet = buffer->Packet;
    uint16_t length = buffer->Length;
    ARPInfo info;

    // The fixed part of the packet is 8 octets, and it is followed by four addresses
    if (length < 8)
    {
        return;
    }

    info.hardwareType = Unpack16(packet, 0);
    info.protocolType = Unpack16(packet, 2);
    info.hardwareSize = Unpack8(packet, 4);
    info.protocolSize = Unpack8(packet, 5);
    info.opType = Unpack16(packet, 6);

    // RFC 826, packet reception: do I have the hardware type, and do I speak the protocol type? The
    // address lengths may be checked too, and are, so the addresses are only read where they are.
    if (info.hardwareType != 1 /* Ethernet */ || info.protocolType != 0x0800 /* IP */ ||
        info.hardwareSize != MAC.AddressSize() || info.protocolSize != IP.AddressSize())
    {
        return;
    }
    if (length < 8 + 2 * info.hardwareSize + 2 * info.protocolSize)
    {
        return;
    }

    info.senderHardwareAddress = &packet[8];
    info.senderProtocolAddress = &packet[8 + info.hardwareSize];
    info.targetHardwareAddress = &packet[8 + info.hardwareSize + info.protocolSize];
    info.targetProtocolAddress = &packet[8 + 2 * info.hardwareSize + info.protocolSize];

    // If the sender is already in the table its hardware address is replaced, whoever the packet
    // is for
    bool merged = (LocateProtocolAddress(info.senderProtocolAddress) >= 0);
    if (merged)
    {
        Add(info.senderProtocolAddress, info.senderHardwareAddress);
    }

    // Am I the target protocol address? A stack that has no address yet is not.
    if (!IP.IsConfigured() ||
        !AddressCompare(info.targetProtocolAddress, IP.GetUnicastAddress(), IP.AddressSize()))
    {
        return;
    }

    if (!merged)
    {
        Add(info.senderProtocolAddress, info.senderHardwareAddress);
    }

    // Only now is the opcode looked at
    if (info.opType == 1)
    {
        // A request, so answer it
        SendReply(info);
    }
    else if (info.opType == 2)
    {
        // A reply, so packets that were waiting for this address can be sent
        IP.Retry();
    }
}

void ProtocolARP::Add(const uint8_t* protocolAddress, const uint8_t* hardwareAddress)
{
    int index;
    uint32_t i;
    int oldest;

    index = LocateProtocolAddress(protocolAddress);
    if (index >= 0)
    {
        // Found entry in table. The new hardware address supersedes the old one (RFC 826), and
        // the entry's age starts again.
        for (size_t j = 0; j < MAC.AddressSize(); j++)
        {
            Cache[index].MACAddress[j] = hardwareAddress[j];
        }
        Cache[index].Age = 1;
    }
    else
    {
        // Not already in table;
        for (i = 0; i < ARPCacheSize; i++)
        {
            if (Cache[i].Age == 0)
            {
                // Entry not used, take it
                break;
            }
        }
        if (i == ARPCacheSize)
        {
            // Table is full, steal the oldest entry
            oldest = 0;
            for (i = 1; i < ARPCacheSize; i++)
            {
                if (Cache[i].Age > Cache[oldest].Age)
                {
                    oldest = i;
                }
            }
            i = oldest;
        }

        // At this point i is the entry we want to use
        Cache[i].Age = 1;
        for (size_t j = 0; j < IP.AddressSize(); j++)
        {
            Cache[i].IPv4Address[j] = protocolAddress[j];
        }
        for (size_t j = 0; j < MAC.AddressSize(); j++)
        {
            Cache[i].MACAddress[j] = hardwareAddress[j];
        }
    }

    // Age the list
    for (i = 0; i < ARPCacheSize; i++)
    {
        if (Cache[i].Age != 0)
        {
            Cache[i].Age++;
            if (Cache[i].Age == 0xFF)
            {
                // Flush this entry
                Cache[i].Age = 0;
            }
        }
    }
}

void ProtocolARP::dump_info(os::TextOutput output, void* context) const
{
    os::Print(output, context, "ARP Cache:\n");
    for (int i = 0; i < ARPCacheSize; i++)
    {
        char ip[16];
        snprintf(ip,
                 sizeof(ip),
                 "%d.%d.%d.%d",
                 (int)Cache[i].IPv4Address[0],
                 (int)Cache[i].IPv4Address[1],
                 (int)Cache[i].IPv4Address[2],
                 (int)Cache[i].IPv4Address[3]);
        os::Print(output,
                  context,
                  "   %-19s%02x:%02x:%02x:%02x:%02x:%02x   age = %d\n",
                  ip,
                  (unsigned)Cache[i].MACAddress[0],
                  (unsigned)Cache[i].MACAddress[1],
                  (unsigned)Cache[i].MACAddress[2],
                  (unsigned)Cache[i].MACAddress[3],
                  (unsigned)Cache[i].MACAddress[4],
                  (unsigned)Cache[i].MACAddress[5],
                  (int)Cache[i].Age);
    }
}

void ProtocolARP::SendReply(const ARPInfo& info)
{
    int offset = 0;
    DataBuffer* txBuffer = MAC.GetTxBuffer();
    if (txBuffer == nullptr)
    {
        printf("ARP failed to get tx buffer\n");
        return;
    }

    offset = Pack16(txBuffer->Packet, offset, info.hardwareType);
    offset = Pack16(txBuffer->Packet, offset, info.protocolType);
    offset = Pack8(txBuffer->Packet, offset, info.hardwareSize);
    offset = Pack8(txBuffer->Packet, offset, info.protocolSize);
    offset = Pack16(txBuffer->Packet, offset, 2); // ARP Reply
    offset = PackBytes(txBuffer->Packet, offset, MAC.GetUnicastAddress(), info.hardwareSize);
    offset = PackBytes(txBuffer->Packet, offset, IP.GetUnicastAddress(), info.protocolSize);
    offset = PackBytes(txBuffer->Packet, offset, info.senderHardwareAddress, info.hardwareSize);
    offset = PackBytes(txBuffer->Packet, offset, info.senderProtocolAddress, info.protocolSize);
    txBuffer->Length = offset;

    MAC.Transmit(txBuffer, info.senderHardwareAddress, 0x0806);
}

void ProtocolARP::SendRequest(const uint8_t* targetIP)
{
    ARPRequest.Initialize(&MAC);

    // This is normally done by the mac layer
    // but this buffer is reserved by arp and not allocated from the mac
    ARPRequest.Packet += MAC.HeaderSize();
    ARPRequest.Remainder -= MAC.HeaderSize();

    ARPRequest.Disposable = false;

    size_t offset = 0;
    offset = Pack16(ARPRequest.Packet, offset, 0x0001); // Hardware Type
    offset = Pack16(ARPRequest.Packet, offset, 0x0800); // Protocol Type
    offset = Pack8(ARPRequest.Packet, offset, 6);       // Hardware Size
    offset = Pack8(ARPRequest.Packet, offset, 4);       // Protocol Size
    offset = Pack16(ARPRequest.Packet, offset, 0x0001); // Op

    // Sender's Hardware Address
    offset = PackBytes(ARPRequest.Packet, offset, MAC.GetUnicastAddress(), 6);

    // Sender's Protocol Address
    offset = PackBytes(ARPRequest.Packet, offset, IP.GetUnicastAddress(), 4);

    // Target's Hardware Address
    offset = PackFill(ARPRequest.Packet, offset, 0, 6);

    // Target's Protocol Address
    ARPRequest.Length = PackBytes(ARPRequest.Packet, offset, targetIP, 4);

    MAC.Transmit(&ARPRequest, MAC.GetBroadcastAddress(), 0x0806);
}

const uint8_t* ProtocolARP::Protocol2Hardware(const uint8_t* protocolAddress)
{
    int index;
    const uint8_t* rc = nullptr;

    if (IsBroadcast(protocolAddress))
    {
        rc = MAC.GetBroadcastAddress();
    }
    else
    {
        if (!IsLocal(protocolAddress))
        {
            protocolAddress = IP.GetGatewayAddress();
        }
        index = LocateProtocolAddress(protocolAddress);

        if (index != -1)
        {
            rc = Cache[index].MACAddress;
        }
        else
        {
            SendRequest(protocolAddress);
        }
    }
    return rc;
}

bool ProtocolARP::IsBroadcast(const uint8_t* protocolAddress)
{
    return IP.IsBroadcastAddress(protocolAddress);
}

bool ProtocolARP::IsLocal(const uint8_t* protocolAddress)
{
    uint32_t i;

    for (i = 0; i < IP.AddressSize(); i++)
    {
        if ((protocolAddress[i] & IP.GetSubnetMask()[i]) !=
            (IP.GetUnicastAddress()[i] & IP.GetSubnetMask()[i]))
        {
            break;
        }
    }

    return i == IP.AddressSize();
}

int ProtocolARP::LocateProtocolAddress(const uint8_t* protocolAddress)
{
    int i;
    int j;

    for (i = 0; i < ARPCacheSize; i++)
    {
        // Go through the address backwards since least significant byte is most
        // likely to be unique
        for (j = IP.AddressSize() - 1; j >= 0; j--)
        {
            if (Cache[i].IPv4Address[j] != protocolAddress[j])
            {
                break;
            }
        }
        if (j == -1)
        {
            // found
            return i;
        }
    }

    return -1;
}

} // namespace tinytcp
