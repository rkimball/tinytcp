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

#include <cinttypes>
#include "Address.h"
#include "DataBuffer.h"
#include "osMutex.h"

class ARPCacheEntry : public Address
{
public:
   uint8_t Age;
};

// HardwareType - 2 bytes
// ProtocolType - 2 bytes
// HardwareSize - 1 byte, size int bytes of HardwareAddress fields
// ProtocolSize - 1 byte, size int bytes of ProtocolAddress fields
// SenderHardwareAddress - HardwareSize bytes
// SenderProtocolAddress - ProtocolSize bytes
// TargetHardwareAddress - HardwareSize bytes
// TargetProtocolAddress - ProtocolSize bytes

class ProtocolARP
{
public:
   static void Initialize(){};
   
   static void ProcessRx( DataBuffer* );

   static void Add( uint8_t* protocolAddress, uint8_t* hardwareAddress );

   static uint8_t* Protocol2Hardware( uint8_t* protocolAddress );
   static bool IsLocal( uint8_t* protocolAddress );
   static bool IsBroadcast( uint8_t* protocolAddress );

   static void Show();

private:
   static void SendReply( uint8_t* packet, int length );
   static void SendRequest( uint8_t* targetIP );
   static int LocateProtocolAddress( uint8_t* protocolAddress );

   static const uint8_t CacheSize = 5;
   static DataBuffer ARPRequest;

   static ARPCacheEntry Cache[];

   ProtocolARP();
   ProtocolARP( ProtocolARP& );
};

#endif

