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

#pragma once

#include <inttypes.h>
#include <iostream>

#include "DataBuffer.hpp"
#include "InterfaceMAC.hpp"
#include "osQueue.hpp"

class ProtocolARP;
class ProtocolICMP;
class ProtocolTCP;
class ProtocolUDP;

class ProtocolIPv4
{
private:
    friend class TCPConnection;
    static const int ADDRESS_SIZE = 4;

public:
    struct AddressInfo
    {
        bool     DataValid;
        uint8_t  Address[ADDRESS_SIZE];
        uint32_t IpAddressLeaseTime;
        uint32_t RenewTime;
        uint32_t RebindTime;
        uint8_t  SubnetMask[ADDRESS_SIZE];
        uint8_t  Gateway[ADDRESS_SIZE];
        uint8_t  DomainNameServer[ADDRESS_SIZE];
        uint8_t  BroadcastAddress[ADDRESS_SIZE];
    };

    ProtocolIPv4(InterfaceMAC&, ProtocolARP&, ProtocolICMP&, ProtocolTCP&, ProtocolUDP&);
    void Initialize();

    void ProcessRx(DataBuffer*);

    void Transmit(DataBuffer*, uint8_t protocol, const uint8_t* targetIP, const uint8_t* sourceIP);
    void Retransmit(DataBuffer*);

    void Retry();

    size_t         AddressSize();
    const uint8_t* GetUnicastAddress();
    const uint8_t* GetBroadcastAddress();
    const uint8_t* GetGatewayAddress();
    const uint8_t* GetSubnetMask();
    void SetAddressInfo(const AddressInfo& info);

    DataBuffer* GetTxBuffer(InterfaceMAC*);
    void        FreeTxBuffer(DataBuffer*);
    void        FreeRxBuffer(DataBuffer*);

    static size_t header_size() { return 20; }

    friend std::ostream& operator<<(std::ostream&, const ProtocolIPv4&);

private:
    bool IsLocal(const uint8_t* addr);

    uint16_t PacketID;
    void*    TxBuffer[TX_BUFFER_COUNT];
    osQueue  UnresolvedQueue;

    AddressInfo Address;

    InterfaceMAC& MAC;
    ProtocolARP&  ARP;
    ProtocolICMP& ICMP;
    ProtocolTCP&  TCP;
    ProtocolUDP&  UDP;

    ProtocolIPv4();
    ProtocolIPv4(ProtocolIPv4&);
};
