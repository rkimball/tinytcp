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

#ifndef PROTOCOLARP_H
#define PROTOCOLARP_H

#include <inttypes.h>
#include "DataBuffer.h"
#include "osMutex.h"
#include "InterfaceMAC.h"
#include "ProtocolIPv4.h"

class ARPCacheEntry
{
public:
   ARPCacheEntry();
   uint8_t Age;
   uint8_t IPv4Address[ 4 ];
   uint8_t MACAddress[ 6 ];
};

// HardwareType - 2 bytes
// ProtocolType - 2 bytes
// HardwareSize - 1 byte, size int bytes of HardwareAddress fields
// IPv4AddressSize - 1 byte, size int bytes of ProtocolAddress fields
// SenderHardwareAddress - HardwareSize bytes
// SenderProtocolAddress - IPv4AddressSize bytes
// TargetHardwareAddress - HardwareSize bytes
// TargetProtocolAddress - IPv4AddressSize bytes

class ProtocolARP
{
public:
   ProtocolARP( InterfaceMAC& mac, ProtocolIPv4& ip );
   void Initialize();
   
   void ProcessRx( const DataBuffer* );

   void Add( const uint8_t* protocolAddress, const uint8_t* hardwareAddress );

   const uint8_t* Protocol2Hardware( const uint8_t* protocolAddress );
   bool IsLocal( const uint8_t* protocolAddress );
   bool IsBroadcast( const uint8_t* protocolAddress );

   void Show( osPrintfInterface* pfunc );

private:
   struct ARPInfo
   {
      uint16_t hardwareType;
      uint16_t protocolType;
      uint8_t  hardwareSize;
      uint8_t  protocolSize;
      uint16_t opType;
      uint8_t* senderHardwareAddress;
      uint8_t* senderProtocolAddress;
      uint8_t* targetHardwareAddress;
      uint8_t* targetProtocolAddress;
   };

   void SendReply( const ARPInfo& info );
   void SendRequest( const uint8_t* targetIP );
   int LocateProtocolAddress( const uint8_t* protocolAddress );

   DataBuffer ARPRequest;

   ARPCacheEntry Cache[ ARPCacheSize ];

   InterfaceMAC& MAC;
   ProtocolIPv4&        IP;

   ProtocolARP();
   ProtocolARP( ProtocolARP& );
};

#endif

