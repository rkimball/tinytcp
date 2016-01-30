//----------------------------------------------------------------------------
// Copyright( c ) 2016, Robert Kimball
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

#ifndef PROTOCOLDHCP_H
#define PROTOCOLDHCP_H

#include <inttypes.h>

#include "DataBuffer.h"

// UDP Src = 0.0.0.0 sPort = 68
// Dest = 255.255.255.255 dPort = 67
struct DHCPDISCOVER
{
   uint8_t op = 0x01;
   uint8_t htype = 0x01;
   uint8_t hlen = 0x06;
   uint8_t hops = 0x00;
   uint32_t xid = 0x3903F326;
   uint16_t secs = 0x0000;
   uint16_t flags = 0x8000;
   uint32_t ciaddr = 0x00000000; // (Client IP address)
   uint32_t yiaddr = 0x00000000; // (Your IP address)
   uint32_t siaddr = 0x00000000; // (Server IP address)
   uint32_t giaddr = 0x00000000; // (Gateway IP address)

   uint8_t chaddr[ 16 ]; // (Client hardware address)
   uint8_t sname[ 64 ];
   uint8_t file[ 128 ];

   uint32_t magic;
   //192 octets of 0s, or overflow space for additional options.BOOTP legacy
   //Magic cookie
   //0x63825363
   //DHCP Options
   //DHCP option 53: DHCP Discover
   //DHCP option 50 : 192.168.1.100 requested
   //DHCP option 55 : Parameter Request List :
   //Request Subnet Mask( 1 ), Router( 3 ), Domain Name( 15 ), Domain Name Server( 6 )
};

class ProtocolDHCP
{
public:
   static void ProcessRx( DataBuffer* buffer );
   static void Discover();
   static void SendRequest( uint8_t messageType, const uint8_t* serverAddress, const uint8_t* requestAddress );
   static void test();
private:
   static DataBuffer Buffer;
   static int PendingXID;
};

#endif