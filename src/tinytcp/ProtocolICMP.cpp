//----------------------------------------------------------------------------
// Copyright(c) 2015-2021, Robert Kimball
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
    uint8_t code;
    DataBuffer* txBuffer;
    uint16_t i;

    type = buffer->Packet[0];
    code = buffer->Packet[1];

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
        break;
    default: break;
    }
}
