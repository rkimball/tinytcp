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

#ifndef PROTOCOLIP_H
#define PROTOCOLIP_H

#include <cinttypes>
#include "osQueue.h"
#include "DataBuffer.h"

#define IP_HEADER_SIZE (20)

class ProtocolIP
{
public:
   static void Initialize();

   static void ProcessRx( DataBuffer*, uint8_t* hardwareAddress );

   static void Transmit( DataBuffer*, uint8_t protocol, uint8_t* targetIP );
   static void Retransmit( DataBuffer* );

   static void Retry();
   
   static DataBuffer* GetTxBuffer();

private:

   static uint16_t PacketID;

   static osQueue UnresolvedQueue;

   ProtocolIP();
   ProtocolIP( ProtocolIP& );
};

#endif
