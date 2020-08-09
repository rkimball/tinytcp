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
#include "FCS.hpp"
#include "ProtocolDHCP.hpp"
#include "ProtocolIPv4.hpp"
#include "ProtocolUDP.hpp"
#include "Utility.hpp"

using namespace std;

//============================================================================
//
//============================================================================

ProtocolUDP::ProtocolUDP(ProtocolIPv4& ip, ProtocolDHCP& dhcp)
    : IP(ip)
    , DHCP(dhcp)
{
}

//============================================================================
//
//============================================================================

DataBuffer* ProtocolUDP::GetTxBuffer(InterfaceMAC* mac)
{
    DataBuffer* buffer;

    buffer = IP.GetTxBuffer(mac);
    if (buffer != 0)
    {
        buffer->Packet += header_size();
        buffer->Remainder -= header_size();
    }

    return buffer;
}

//============================================================================
//
//============================================================================

void ProtocolUDP::ProcessRx(DataBuffer* buffer, const uint8_t* sourceIP, const uint8_t* targetIP)
{
    uint16_t sourcePort = Unpack16(buffer->Packet, 0);
    uint16_t targetPort = Unpack16(buffer->Packet, 2);

    buffer->Packet += header_size();
    buffer->Remainder -= header_size();

    switch (targetPort)
    {
    case 68: // DHCP Client Port
        DHCP.ProcessRx(buffer);
        break;
    default:
        //printf( "Rx UDP target port %d\n", targetPort );
        break;
    }
}

//============================================================================
//
//============================================================================

void ProtocolUDP::Transmit(DataBuffer* buffer,
                           const uint8_t* targetIP,
                           uint16_t targetPort,
                           const uint8_t* sourceIP,
                           uint16_t sourcePort)
{
    buffer->Packet -= header_size();
    buffer->Remainder += header_size();

    Pack16(buffer->Packet, 0, sourcePort);
    Pack16(buffer->Packet, 2, targetPort);
    Pack16(buffer->Packet, 4, buffer->Length);

    // Calculate checksum
    uint8_t pheader_tmp[4];
    pheader_tmp[0] = 0;
    pheader_tmp[1] = 0x11;
    Pack16(pheader_tmp, 2, buffer->Length);
    uint32_t acc = 0;
    FCS::ChecksumAdd(sourceIP, 4, acc);
    acc = FCS::ChecksumAdd(targetIP, 4, acc);
    acc = FCS::ChecksumAdd(pheader_tmp, 4, acc);
    acc = FCS::ChecksumAdd(buffer->Packet, buffer->Length, acc);
    Pack16(buffer->Packet, 6, FCS::ChecksumComplete(acc));

    IP.Transmit(buffer, 0x11, targetIP, sourceIP);
}
