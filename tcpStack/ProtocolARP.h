//----------------------------------------------------------------------------
//  Copyright 2015 Robert Kimball
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http ://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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

