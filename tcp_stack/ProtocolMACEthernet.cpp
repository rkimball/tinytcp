//----------------------------------------------------------------------------
// Copyright(c) 2015-2020, Robert Kimball
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

#include <iostream>
#include <stdio.h>

#include "ProtocolARP.hpp"
#include "ProtocolIPv4.hpp"
#include "ProtocolMACEthernet.hpp"
#include "Utility.hpp"

// Destination - 6 bytes
// Source - 6 bytes
// FrameType - 2 bytes

ProtocolMACEthernet::ProtocolMACEthernet(ProtocolARP& arp, ProtocolIPv4& ipv4)
    : TxBufferQueue("Tx", TX_BUFFER_COUNT, TxBufferBuffer)
    , RxBufferQueue("Rx", RX_BUFFER_COUNT, RxBufferBuffer)
    , QueueEmptyEvent("MACEthernet")
    , TxHandler(nullptr)
    , ARP(arp)
    , IPv4(ipv4)
{
    int i;

    BroadcastAddress[0] = 0xFF;
    BroadcastAddress[1] = 0xFF;
    BroadcastAddress[2] = 0xFF;
    BroadcastAddress[3] = 0xFF;
    BroadcastAddress[4] = 0xFF;
    BroadcastAddress[5] = 0xFF;

    for (i = 0; i < TX_BUFFER_COUNT; i++)
    {
        TxBufferQueue.Put(&TxBuffer[i]);
    }
    for (i = 0; i < RX_BUFFER_COUNT; i++)
    {
        RxBufferQueue.Put(&RxBuffer[i]);
    }
}

void ProtocolMACEthernet::RegisterDataTransmitHandler(DataTransmitHandler handler)
{
    TxHandler = handler;
}

bool ProtocolMACEthernet::IsLocalAddress(const uint8_t* addr)
{
    return AddressCompare(UnicastAddress, addr, 6) || AddressCompare(BroadcastAddress, addr, 6);
}

void ProtocolMACEthernet::ProcessRx(uint8_t* buffer, int actualLength)
{
    uint16_t type;
    DataBuffer* packet = static_cast<DataBuffer*>(RxBufferQueue.Get());
    int i;
    int length =
        (DATA_BUFFER_PAYLOAD_SIZE < actualLength ? DATA_BUFFER_PAYLOAD_SIZE : actualLength);

    if (packet == nullptr)
    {
        printf("ProtocolMACEthernet::ProcessRx Out of receive buffers\n");
        return;
    }

    if (length > DATA_BUFFER_PAYLOAD_SIZE)
    {
        // printf("ProtocolMACEthernet::ProcessRx Rx data overrun %d, %d\n",
        //        length,
        //        DATA_BUFFER_PAYLOAD_SIZE);
        RxBufferQueue.Put(packet);
        return;
    }

    packet->Initialize(this);

    for (i = 0; i < length; i++)
    {
        packet->Packet[i] = buffer[i];
    }
    packet->Length = length;

    type = Unpack16(packet->Packet, 12);

    // Check if the MAC Address is destined for me
    if (IsLocalAddress(packet->Packet))
    {
        // DumpData( buffer, length, printf );
        if (actualLength > length)
        {
            // printf("ProtocolMACEthernet::ProcessRx Rx data overrun %d, %d\n",
            //        length,
            //        DATA_BUFFER_PAYLOAD_SIZE);
            // printf("Unicast type 0x%04X\n", type);
            RxBufferQueue.Put(packet);
            return;
        }
        // Unicast
        packet->Packet += header_size();
        packet->Length -= header_size();

        switch (type)
        {
        case 0x0800: // IP
            IPv4.ProcessRx(packet);
            break;
        case 0x0806: // ARP
            ARP.ProcessRx(packet);
            break;
        default:
            // printf( "Unsupported Unicast type 0x%04X\n", type );
            break;
        }
    }

    if (packet->Disposable)
    {
        RxBufferQueue.Put(packet);
    }
}

DataBuffer* ProtocolMACEthernet::GetTxBuffer()
{
    DataBuffer* buffer;

    while ((buffer = static_cast<DataBuffer*>(TxBufferQueue.Get())) == nullptr)
    {
        QueueEmptyEvent.Wait(__FILE__, __LINE__);
    }
    if (buffer != nullptr)
    {
        buffer->Initialize(this);
        buffer->Packet += header_size();
        buffer->Remainder -= header_size();
    }

    return buffer;
}

void ProtocolMACEthernet::FreeTxBuffer(DataBuffer* buffer)
{
    TxBufferQueue.Put(buffer);
    QueueEmptyEvent.Notify();
}

void ProtocolMACEthernet::FreeRxBuffer(DataBuffer* buffer)
{
    RxBufferQueue.Put(buffer);
}

void ProtocolMACEthernet::Transmit(DataBuffer* buffer, const uint8_t* targetMAC, uint16_t type)
{
    buffer->Packet -= header_size();
    buffer->Length += header_size();

    size_t offset = 0;
    offset = PackBytes(buffer->Packet, offset, targetMAC, 6);
    offset = PackBytes(buffer->Packet, offset, UnicastAddress, 6);
    offset = Pack16(buffer->Packet, offset, type);

    offset += buffer->Length;
    while (buffer->Length < 60)
    {
        buffer->Packet[buffer->Length++] = 0;
    }

    if (TxHandler)
    {
        TxHandler(buffer->Packet, buffer->Length);
    }

    if (buffer->Disposable)
    {
        TxBufferQueue.Put(buffer);
    }
}

void ProtocolMACEthernet::Retransmit(DataBuffer* buffer)
{
    if (TxHandler)
    {
        TxHandler(buffer->Packet, buffer->Length);
    }

    if (buffer->Disposable)
    {
        TxBufferQueue.Put(buffer);
    }
}

size_t ProtocolMACEthernet::AddressSize() const
{
    return ADDRESS_SIZE;
}

size_t ProtocolMACEthernet::HeaderSize() const
{
    return header_size();
}

const uint8_t* ProtocolMACEthernet::GetUnicastAddress() const
{
    return UnicastAddress;
}

const uint8_t* ProtocolMACEthernet::GetBroadcastAddress() const
{
    return BroadcastAddress;
}

std::ostream& operator<<(std::ostream& out, const ProtocolMACEthernet& obj)
{
    out << "MAC Configuration\n";
    out << "   Ethernet Unicast MAC Address: " << macaddrtoa(obj.GetUnicastAddress()) << "\n";
    out << "   Ethernet Broadcast MAC Address: " << macaddrtoa(obj.GetBroadcastAddress()) << "\n";
    return out;
}

void ProtocolMACEthernet::SetUnicastAddress(uint8_t* addr)
{
    for (int i = 0; i < 6; i++)
    {
        UnicastAddress[i] = addr[i];
    }
}
