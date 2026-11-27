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

#include "FCS.hpp"
#include "ProtocolICMP.hpp"
#include "ProtocolIPv4.hpp"
#include "Utility.hpp"

namespace tinytcp
{

// Type - 8 bits
// Code - 8 bits
// Checksum - 16 bits

ProtocolICMP::ProtocolICMP(ProtocolIPv4& ip)
    : IP(ip)
{
}

void ProtocolICMP::ProcessRx(DataBuffer* buffer, const uint8_t* remoteIP, const uint8_t*)
{
    uint8_t type;
    DataBuffer* txBuffer;
    uint16_t i;

    // Every message starts with a type, a code, a checksum and 4 more octets, which is 8 in all.
    // The checksum of RFC 792 is over the whole message, and a message that has its checksum in it
    // adds up to zero. A message that is too short or fails the checksum is damaged and is
    // discarded.
    if (buffer->Length < 8 || FCS::Checksum(buffer->Packet, buffer->Length) != 0)
    {
        return;
    }

    type = buffer->Packet[0];

    switch (type)
    {
    case 8: // echo request
        txBuffer = IP.GetTxBuffer(buffer->MAC);
        if (txBuffer && buffer->Length <= txBuffer->Remainder)
        {
            for (i = 0; i < buffer->Length; i++)
            {
                txBuffer->Packet[i] = buffer->Packet[i];
            }
            txBuffer->Packet[0] = 0;
            Pack16(txBuffer->Packet, 2, 0); // clear the checksum
            i = FCS::Checksum(txBuffer->Packet, buffer->Length);
            Pack16(txBuffer->Packet, 2, i); // set the checksum
            txBuffer->Length = buffer->Length;
            IP.Transmit(txBuffer, 0x01, remoteIP, IP.GetUnicastAddress());
        }
        else if (txBuffer)
        {
            IP.FreeTxBuffer(txBuffer);
        }
        break;
    default: break;
    }
}

// The ICMP types that are errors. RFC 1122 says an error is never sent because of one.
static bool IsIcmpError(uint8_t type)
{
    return type == 3 || type == 4 || type == 5 || type == 11 || type == 12;
}

// Whether an address names one host, which an error can be sent to. Zero, loopback, broadcast,
// multicast and class E addresses do not.
static bool IsSingleHost(ProtocolIPv4& ip, const uint8_t* addr)
{
    bool zero = addr[0] == 0 && addr[1] == 0 && addr[2] == 0 && addr[3] == 0;
    return !zero && addr[0] != 127 && addr[0] < 224 && !ip.IsBroadcastAddress(addr);
}

void ProtocolICMP::SendDestinationUnreachable(uint8_t code,
                                              const DataBuffer* received,
                                              const uint8_t* datagram,
                                              uint16_t length)
{
    const uint8_t* source = &datagram[12];
    const uint8_t* destination = &datagram[16];
    uint16_t headerLength = (datagram[0] & 0x0F) * 4;

    // RFC 1122 section 3.2.2: an ICMP error is not sent because of a datagram that came in a link
    // layer broadcast, one to an IP broadcast or multicast address, one that is not the first
    // fragment, one from an address that is not a single host, or another ICMP error
    if (received->LinkBroadcast || IP.IsBroadcastAddress(destination) || destination[0] >= 224 ||
        (Unpack16(datagram, 6) & 0x1FFF) != 0 || !IsSingleHost(IP, source))
    {
        return;
    }
    if (datagram[9] == 0x01 && length > headerLength && IsIcmpError(datagram[headerLength]))
    {
        return;
    }

    DataBuffer* txBuffer = IP.GetTxBuffer(received->MAC);
    if (txBuffer == nullptr)
    {
        return;
    }

    // RFC 792: the message is a type, a code, a checksum and 4 unused octets that are zero, then
    // the IP header of the datagram and the first 64 bits of its data
    uint16_t quoted = (length < headerLength + 8) ? length : headerLength + 8;
    if (8 + quoted > txBuffer->Remainder)
    {
        IP.FreeTxBuffer(txBuffer);
        return;
    }
    uint8_t* message = txBuffer->Packet;
    message[0] = 3; // destination unreachable
    message[1] = code;
    Pack16(message, 2, 0); // the checksum, filled in below
    Pack32(message, 4, 0); // unused
    for (uint16_t i = 0; i < quoted; i++)
    {
        message[8 + i] = datagram[i];
    }
    txBuffer->Length = 8 + quoted;
    Pack16(message, 2, FCS::Checksum(message, txBuffer->Length));
    IP.Transmit(txBuffer, 0x01, source, IP.GetUnicastAddress());
}

} // namespace tinytcp
