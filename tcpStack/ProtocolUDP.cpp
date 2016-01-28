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

#include <stdio.h>
#include "ProtocolIP.h"
#include "ProtocolUDP.h"
#include "Utility.h"
#include "FCS.h"

//============================================================================
//
//============================================================================

DataBuffer* ProtocolUDP::GetTxBuffer()
{
   DataBuffer*   buffer;

   buffer = ProtocolIP::GetTxBuffer();
   if( buffer != 0 )
   {
      buffer->Packet += UDP_HEADER_SIZE;
      buffer->Remainder -= UDP_HEADER_SIZE;
   }

   return buffer;
}

//============================================================================
//
//============================================================================

void ProtocolUDP::ProcessRx( DataBuffer* buffer, uint8_t* sourceIP, uint8_t* targetIP )
{
   printf( "got udp, source %d.%d.%d.%d, target %d.%d.%d.%d\n",
      sourceIP[ 0 ], sourceIP[ 1 ], sourceIP[ 2 ], sourceIP[ 3 ],
      targetIP[ 0 ], targetIP[ 1 ], targetIP[ 2 ], targetIP[ 3 ]
      );
}

//============================================================================
//
//============================================================================

void ProtocolUDP::Transmit( DataBuffer* buffer, uint8_t* targetIP, uint16_t targetPort, uint8_t* sourceIP, uint16_t sourcePort )
{
   uint8_t* packet;

   buffer->Packet -= UDP_HEADER_SIZE;
   buffer->Length += UDP_HEADER_SIZE;
   Datagram* datagram = (Datagram*)(buffer->Packet);
   datagram->sourcePort = hton16( sourcePort );
   datagram->targetPort = hton16( targetPort );
   datagram->length = hton16( buffer->Length );

   // Calculate checksum
   uint8_t pheader_tmp[ 4 ];
   pheader_tmp[ 0 ] = 0;
   pheader_tmp[ 1 ] = 0x11;
   pheader_tmp[ 2 ] = buffer->Length >> 8;
   pheader_tmp[ 3 ] = buffer->Length & 0xFF;
   uint32_t acc = 0;
   FCS::ChecksumAdd( sourceIP, 4, acc );
   acc = FCS::ChecksumAdd( targetIP, 4, acc );
   acc = FCS::ChecksumAdd( pheader_tmp, 4, acc );
   acc = FCS::ChecksumAdd( buffer->Packet, buffer->Length, acc );
   datagram->checksum = hton16( FCS::ChecksumComplete(acc) );

   printf( "buffer length %d\n", buffer->Length );
   printf( "checksum 0x%04X\n", datagram->checksum );

   ProtocolIP::Transmit( buffer, 0x11, targetIP, sourceIP );
}