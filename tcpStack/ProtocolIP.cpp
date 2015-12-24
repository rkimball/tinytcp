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

#include "ProtocolMACEthernet.h"
#include "ProtocolARP.h"
#include "ProtocolIP.h"
#include "ProtocolICMP.h"
#include "ProtocolTCP.h"
#include "Address.h"
#include "FCS.h"
#include "DataBuffer.h"
#include "NetworkInterface.h"

uint16_t ProtocolIP::PacketID;

extern AddressConfiguration Config;

osQueue ProtocolIP::UnresolvedQueue;

// Version - 4 bits
// Header Length - 4 bits
// Type of Service - 8 bits
// TotalLength - 16 bits
// Identification - 16 bits
// Flags - 3 bits
// FragmentOffset - 13 bits
// TTL - 8 bits
// Protocol - 8 bits
// HeaderChecksum - 16 bits

//============================================================================
//
//============================================================================

void ProtocolIP::Initialize()
{
   ProtocolTCP::Initialize();
   ProtocolICMP::Initialize();

   UnresolvedQueue.Initialize( TX_BUFFER_COUNT, "IP" );
}

//============================================================================
//
//============================================================================

void ProtocolIP::ProcessRx( DataBuffer* buffer, uint8_t* hardwareAddress )
{
   uint8_t headerLength;
   uint8_t protocol;
   uint8_t* sourceIP;
   uint8_t* targetIP;
   uint8_t* packet = buffer->Packet;
   uint16_t length = buffer->Length;
   uint16_t dataLength;

   headerLength = (packet[ 0 ] & 0x0F) * 4;
   dataLength = packet[ 2 ] << 8 | packet[ 3 ];
   protocol = packet[ 9 ];
   sourceIP = &packet[ 12 ];
   targetIP = &packet[ 16 ];

   //printf( "IP %d, dataLength %d, header %d\n", buffer->Length, dataLength, headerLength );
   buffer->Packet += headerLength;
   dataLength -= headerLength;
   buffer->Length = dataLength;

   switch( protocol )
   {
   case 0x01:  // ICMP
      ProtocolICMP::ProcessRx( buffer, sourceIP );
      break;
   case 0x02:  // IGMP
      break;
   case 0x06:  // TCP
      ProtocolTCP::ProcessRx( buffer, sourceIP, targetIP );
      break;
   case 0x11:  // UDP
      break;
   default:
      printf( "Unsupported IP Protocol 0x%02X\n", protocol );
      break;
   }
}

//============================================================================
//
//============================================================================

DataBuffer* ProtocolIP::GetTxBuffer()
{
   DataBuffer*   buffer;

   buffer = ProtocolMACEthernet::GetTxBuffer();
   if( buffer != 0 )
   {
      buffer->Packet += IP_HEADER_SIZE;
      buffer->Remainder -= IP_HEADER_SIZE;
   }
   
   return buffer;
}

//============================================================================
//
//============================================================================

void ProtocolIP::Transmit( DataBuffer* buffer, uint8_t protocol, uint8_t* targetIP )
{
   uint16_t checksum;
   uint8_t* targetMAC;
   uint8_t* packet;
   
   buffer->Packet -= IP_HEADER_SIZE;
   buffer->Length += IP_HEADER_SIZE;
   packet = buffer->Packet;

   packet[ 0 ] = 0x45;     // Version and HeaderSize
   packet[ 1 ] = 0;        // ToS
   packet[ 2 ] = buffer->Length >> 8;
   packet[ 3 ] = buffer->Length & 0xFF;

   PacketID++;
   packet[ 4 ] = PacketID >> 8;
   packet[ 5 ] = PacketID & 0xFF;
   packet[ 6 ] = 0;        // Flags & FragmentOffset
   packet[ 7 ] = 0;        // rest of FragmentOffset

   packet[  8 ] = 32;      // TTL
   packet[  9 ] = protocol;
   packet[ 10 ] = 0;       // Seed Checksum
   packet[ 11 ] = 0;

   packet[ 12 ] = Config.Address.Protocol[ 0 ];
   packet[ 13 ] = Config.Address.Protocol[ 1 ];
   packet[ 14 ] = Config.Address.Protocol[ 2 ];
   packet[ 15 ] = Config.Address.Protocol[ 3 ];
   
   packet[ 16 ] = targetIP[ 0 ];
   packet[ 17 ] = targetIP[ 1 ];
   packet[ 18 ] = targetIP[ 2 ];
   packet[ 19 ] = targetIP[ 3 ];

   checksum = FCS::Checksum( packet, 20 );
   packet[ 10 ] = checksum >> 8;
   packet[ 11 ] = checksum & 0xFF;

   //printf( "Tx to %3d.%3d.%3d.%3d\n",
   //   targetIP[ 0 ],
   //   targetIP[ 1 ],
   //   targetIP[ 2 ],
   //   targetIP[ 3 ] );
   targetMAC = ProtocolARP::Protocol2Hardware( targetIP );
   if( targetMAC != 0 )
   {
      ProtocolMACEthernet::Transmit( buffer, targetMAC, 0x0800 );
   }
   else
   {
      printf( "Could not find MAC for IP\n" );
      UnresolvedQueue.Put( buffer );
   }
}

//============================================================================
//
//============================================================================

void ProtocolIP::Retransmit( DataBuffer* buffer )
{
   ProtocolMACEthernet::Retransmit( buffer );
}

//============================================================================
//
//============================================================================

void ProtocolIP::Retry()
{
   int count;
   DataBuffer* buffer;
   uint8_t* targetMAC;

   count = UnresolvedQueue.GetCount();
   for( int i=0; i<count; i++ )
   {
      buffer = (DataBuffer*)UnresolvedQueue.Get();

      printf( "Retry Tx to %3d.%3d.%3d.%3d\n",
         buffer->Packet[ 16 ],
         buffer->Packet[ 17 ],
         buffer->Packet[ 18 ],
         buffer->Packet[ 19 ] );
      targetMAC = ProtocolARP::Protocol2Hardware( &buffer->Packet[ 16 ] );
      if( targetMAC != 0 )
      {
         printf( "Success\n" );
         ProtocolMACEthernet::Transmit( buffer, targetMAC, 0x0800 );
      }
      else
      {
         printf( "Still could not find MAC for IP\n" );
         UnresolvedQueue.Put( buffer );
      }
   }
}
