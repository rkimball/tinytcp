//----------------------------------------------------------------------------
// Copyright( c ) 2015, Robert Kimball
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

#pragma once

#include <inttypes.h>

#include "DataBuffer.hpp"
#include "InterfaceMAC.hpp"
#include "osEvent.hpp"
#include "osQueue.hpp"

#define MAC_HEADER_SIZE (14)

class ProtocolARP;
class ProtocolIPv4;

class ProtocolMACEthernet : public InterfaceMAC
{
public:
    ProtocolMACEthernet(ProtocolARP&, ProtocolIPv4&);
    void RegisterDataTransmitHandler(DataTransmitHandler);

    void ProcessRx(uint8_t* buffer, int length);

    void Transmit(DataBuffer*, const uint8_t* targetMAC, uint16_t type);
    void Retransmit(DataBuffer* buffer);

    DataBuffer* GetTxBuffer();
    void        FreeTxBuffer(DataBuffer*);
    void        FreeRxBuffer(DataBuffer*);

    size_t AddressSize() const;
    size_t HeaderSize() const;

    const uint8_t* GetUnicastAddress() const;
    const uint8_t* GetBroadcastAddress() const;

    void SetUnicastAddress(uint8_t* addr);

    friend std::ostream& operator<<(std::ostream&, const ProtocolMACEthernet&);

private:
    static const int ADDRESS_SIZE = 6;
    osQueue          TxBufferQueue;
    osQueue          RxBufferQueue;

    osEvent QueueEmptyEvent;

    uint8_t UnicastAddress[ADDRESS_SIZE];
    uint8_t BroadcastAddress[ADDRESS_SIZE];

    DataBuffer TxBuffer[TX_BUFFER_COUNT];
    DataBuffer RxBuffer[RX_BUFFER_COUNT];

    void* TxBufferBuffer[TX_BUFFER_COUNT];
    void* RxBufferBuffer[RX_BUFFER_COUNT];

    DataTransmitHandler TxHandler;
    ProtocolARP&        ARP;
    ProtocolIPv4&       IPv4;

    bool IsLocalAddress(const uint8_t* addr);

    ProtocolMACEthernet(ProtocolMACEthernet&);
    ProtocolMACEthernet();
};
