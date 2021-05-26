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

#pragma once

#include <inttypes.h>

#include "DataBuffer.hpp"

class InterfaceMAC;
class ProtocolIPv4;
class ProtocolUDP;

// UDP Src = 0.0.0.0 sPort = 68
// Dest = 255.255.255.255 dPort = 67

class ProtocolDHCP
{
public:
    ProtocolDHCP(InterfaceMAC& mac, ProtocolIPv4& ip, ProtocolUDP& udp);
    void ProcessRx(DataBuffer* buffer);
    void Discover();
    void SendRequest(uint8_t messageType,
                     const uint8_t* serverAddress,
                     const uint8_t* requestAddress);
    void test();

private:
    DataBuffer Buffer;
    int PendingXID;

    InterfaceMAC& MAC;
    ProtocolIPv4& IP;
    ProtocolUDP& UDP;
};
