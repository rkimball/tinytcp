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

#include <stdio.h>

#include "ProtocolICMP.h"
#include "ProtocolIP.h"
#include "FCS.h"
#include "Utility.h"

// Type - 8 bits
// Code - 8 bits
// Checksum - 16 bits

//============================================================================
//
//============================================================================

void ProtocolICMP::ProcessRx( DataBuffer* buffer, uint8_t* sourceIP )
{
   uint8_t type;
   uint8_t code;
   DataBuffer* txBuffer;
   uint16_t i;

   //printf( "ICMP Payload\n" );
   //DumpData( packet, length, printf );

   type = buffer->Packet[ 0 ];
   code = buffer->Packet[ 1 ];

   switch( type )
   {
   case 8:  // echo request
      txBuffer = ProtocolIP::GetTxBuffer();
      if( txBuffer && buffer->Length <= txBuffer->Remainder )
      {
         txBuffer->Packet[ 0 ] = 0;
         for( i=1; i<buffer->Length; i++ )
         {
            txBuffer->Packet[ i ] = buffer->Packet[ i ];
         }
         txBuffer->Packet[ 2 ] = 0;
         txBuffer->Packet[ 3 ] = 0;
         i = FCS::Checksum( txBuffer->Packet, buffer->Length );
         txBuffer->Packet[ 2 ] = i >> 8;
         txBuffer->Packet[ 3 ] = i & 0xFF;
         txBuffer->Length += buffer->Length + 4;
         ProtocolIP::Transmit( txBuffer, 0x01, sourceIP );
      }
      break;
   default:
      break;
   }
}
