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

#ifndef PROTOCOLMACETHERNET_H
#define PROTOCOLMACETHERNET_H

#include <cinttypes>
#include "DataBuffer.h"
#include "osQueue.h"
#include "NetworkInterface.h"

#define MAC_HEADER_SIZE (14)

class ProtocolMACEthernet
{
public:
   static void Initialize( NetworkInterface* dataInterface );

   static void ProcessRx( uint8_t* buffer, int length );

   static void Transmit( DataBuffer*, uint8_t* targetMAC, uint16_t type );
   static void Retransmit( DataBuffer* buffer );

   static DataBuffer* GetTxBuffer();
   static void FreeTxBuffer( DataBuffer* );
   static void FreeRxBuffer( DataBuffer* );

   static int HeaderSize()
   {
      return MAC_HEADER_SIZE;
   }

private:

   static osQueue TxBufferQueue;
   static osQueue RxBufferQueue;
   static NetworkInterface* DataInterface;

   ProtocolMACEthernet( ProtocolMACEthernet& );
   ProtocolMACEthernet();
};

#endif
