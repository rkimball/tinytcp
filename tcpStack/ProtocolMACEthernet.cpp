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
#include "Address.h"
#include "Utility.h"
#include "ProtocolARP.h"
#include "ProtocolIP.h"
#include "NetworkInterface.h"

#include "osQueue.h"
#include "osEvent.h"

NetworkInterface* ProtocolMACEthernet::DataInterface;

static int DropCount = 0;
#define MAC_DROP_COUNT 20

extern AddressConfiguration Config;

DataBuffer TxBuffer[ TX_BUFFER_COUNT ];
DataBuffer RxBuffer[ RX_BUFFER_COUNT ];

osQueue ProtocolMACEthernet::TxBufferQueue;
osQueue ProtocolMACEthernet::RxBufferQueue;

static osEvent Event( "MACEthernet" );

// Destination - 6 bytes
// Source - 6 bytes
// FrameType - 2 bytes

//============================================================================
//
//============================================================================

void ProtocolMACEthernet::Initialize( NetworkInterface* dataInterface )
{
   int i;

   DataInterface = dataInterface;

   Config.BroadcastMACAddress[ 0 ] = 0xFF;
   Config.BroadcastMACAddress[ 1 ] = 0xFF;
   Config.BroadcastMACAddress[ 2 ] = 0xFF;
   Config.BroadcastMACAddress[ 3 ] = 0xFF;
   Config.BroadcastMACAddress[ 4 ] = 0xFF;
   Config.BroadcastMACAddress[ 5 ] = 0xFF;

   TxBufferQueue.Initialize( TX_BUFFER_COUNT, "Tx" );
   for( i=0; i<TX_BUFFER_COUNT; i++ )
   {
      TxBufferQueue.Put( &TxBuffer[ i ] );
   }
   printf( "Buffer init\n" );
   RxBufferQueue.Initialize( RX_BUFFER_COUNT, "Rx" );
   for( i=0; i<RX_BUFFER_COUNT; i++ )
   {
      printf( "rx buffer\n" );
      RxBufferQueue.Put( &RxBuffer[ i ] );
   }

   ProtocolARP::Initialize();
   ProtocolIP::Initialize();
}

//============================================================================
//
//============================================================================

void ProtocolMACEthernet::ProcessRx( uint8_t* buffer, int actualLength )
{
   uint16_t   type;
   DataBuffer* packet = (DataBuffer*)RxBufferQueue.Get();
   int i;
   int length = (DATA_BUFFER_PAYLOAD_SIZE < actualLength?DATA_BUFFER_PAYLOAD_SIZE:actualLength);

   if( packet == 0 )
   {
      printf( "ProtocolMACEthernet::ProcessRx Out of receive buffers\n" );
      return;
   }

   if( length > DATA_BUFFER_PAYLOAD_SIZE )
   {
      //printf( "ProtocolMACEthernet::ProcessRx Rx data overrun %d, %d\n", length, DATA_BUFFER_PAYLOAD_SIZE );
      RxBufferQueue.Put( packet );
      return;
   }

   packet->Initialize();

   for( i=0; i<length; i++ )
   {
      packet->Packet[ i ] = buffer[ i ];
   }
   packet->Length = length;

   type = packet->Packet[ 12 ] << 8 | packet->Packet[ 13 ];

   // Check if the MAC Address is destined for me
   if( Address::Compare( Config.Address.Hardware, packet->Packet, 6 ) )
   {
      if( actualLength > length )
      {
         //printf( "ProtocolMACEthernet::ProcessRx Rx data overrun %d, %d\n", length, DATA_BUFFER_PAYLOAD_SIZE );
         //printf( "Unicast type 0x%04X\n", type );
         //DumpData( buffer, length, printf );
         RxBufferQueue.Put( packet );
         return;
      }
      // Unicast
      packet->Packet += MAC_HEADER_SIZE;
      packet->Length -= MAC_HEADER_SIZE;

      switch( type )
      {
      case 0x0800:   // IP
         ProtocolIP::ProcessRx( packet, &buffer[ 6 ] );
         break;
      case 0x0806:   // ARP
         ProtocolARP::ProcessRx( packet );
         break;
      default:
         //printf( "Unsupported Unicast type 0x%04X\n", type );
         break;
      }
   }
   else if( Address::Compare( Config.BroadcastMACAddress, packet->Packet, 6 ) )
   {
      // Broadcast
      //printf( "Broadcast 0x%04X\n", type );
      packet->Packet += MAC_HEADER_SIZE;
      packet->Length -= MAC_HEADER_SIZE;
      
      switch( type )
      {
      case 0x0806:   // ARP
         //printf( "Ethernet Rx:\n" );
         //DumpData( buffer, length, printf );
         ProtocolARP::ProcessRx( packet );
         break;
      default:
         //printf( "Unsupported Broadcast type 0x%04X\n", type );
         break;
      }
   }

   if( packet->Disposable )
   {
      RxBufferQueue.Put( packet );
   }
   else
   {
      printf( "not disposable\n" );
   }
}

//============================================================================
//
//============================================================================

DataBuffer* ProtocolMACEthernet::GetTxBuffer()
{
   DataBuffer* buffer;

   while( (buffer = (DataBuffer*)TxBufferQueue.Get()) == 0 )
   {
      Event.Wait( __FILE__, __LINE__ );
   }
   if( buffer != 0 )
   {
      buffer->Initialize();
      buffer->Packet += MAC_HEADER_SIZE;
      buffer->Remainder -= MAC_HEADER_SIZE;
   }

   return buffer;
}

//============================================================================
//
//============================================================================

void ProtocolMACEthernet::FreeTxBuffer( DataBuffer* buffer )
{
   TxBufferQueue.Put( buffer );
   Event.Notify();
}

//============================================================================
//
//============================================================================

void ProtocolMACEthernet::FreeRxBuffer( DataBuffer* buffer )
{
   RxBufferQueue.Put( buffer );
}

//============================================================================
//
//============================================================================

void ProtocolMACEthernet::Transmit( DataBuffer* buffer, uint8_t* targetMAC, uint16_t type )
{
   uint8_t i;
   uint8_t* p;

   buffer->Packet -= MAC_HEADER_SIZE;
   buffer->Length += MAC_HEADER_SIZE;
   
   p = buffer->Packet;

   for( i=0; i<6; i++ )
   {
      *p++ = targetMAC[ i ];
   }

   for( i=0; i<6; i++ )
   {
      *p++ = Config.Address.Hardware[ i ];
   }

   *p++ = type >> 8;
   *p++ = type & 0xFF;

   //for( i=0; i<length; i++ )
   //{
   //   *p++ = buffer[ i ];
   //}

   p += buffer->Length;

   while( buffer->Length < 60 )
   {
      *p++ = 0;
      buffer->Length++;
   }

   DataInterface->TxData( buffer->Packet, buffer->Length );

   if( buffer->Disposable )
   {
      TxBufferQueue.Put( buffer );
   }
}

//============================================================================
//
//============================================================================

void ProtocolMACEthernet::Retransmit( DataBuffer* buffer )
{
   DataInterface->TxData( buffer->Packet, buffer->Length );

   if( buffer->Disposable )
   {
      TxBufferQueue.Put( buffer );
   }
}

