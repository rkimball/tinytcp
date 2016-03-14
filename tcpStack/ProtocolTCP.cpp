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

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "ProtocolTCP.h"
#include "ProtocolIP.h"
#include "ProtocolMACEthernet.h"
#include "Address.h"
#include "FCS.h"
#include "Utility.h"
#include "DataBuffer.h"
#include "osMutex.h"
#include "osTime.h"
#include "NetworkInterface.h"

ProtocolTCP::Connection ProtocolTCP::ConnectionList[ TCP_MAX_CONNECTIONS ];
uint16_t ProtocolTCP::NextPort;

//============================================================================
//
//============================================================================

ProtocolTCP::Connection::Connection() :
   Event( "tcp connection" ),
   HoldingQueueLock( "" )
{
   TxBuffer = 0;
   NewConnection = 0;
   HoldingQueue.Initialize( TX_BUFFER_COUNT, "TCPHolding" );
   RxBufferEmpty = true;
   RxInOffset = 0;
   RxOutOffset = 0;
   CurrentWindow = TCP_RX_WINDOW_SIZE;
}

//============================================================================
//
//============================================================================

ProtocolTCP::Connection::~Connection()
{
}

//============================================================================
//
//============================================================================

void ProtocolTCP::Connection::SendFlags( uint8_t flags )
{
   DataBuffer* buffer = GetTxBuffer();

   if( buffer )
   {
      BuildPacket( buffer, flags );
   }
}

//============================================================================
//
//============================================================================

void ProtocolTCP::Connection::BuildPacket( DataBuffer* buffer, uint8_t flags )
{
   uint8_t* packet;
   uint16_t checksum;
   uint16_t length;

   flags |= FLAG_ACK;

   buffer->Packet -= TCP_HEADER_SIZE;
   packet = buffer->Packet;
   length = buffer->Length;
   if( packet != 0 )
   {
      Pack16( packet, 0, LocalPort );
      Pack16( packet, 2, RemotePort );
      Pack32( packet, 4, SequenceNumber );
      if( (int32_t)(AcknowledgementNumber - LastAck) > 0 )
      {
         // Only advance LastAck if Ack > LastAck
         LastAck = AcknowledgementNumber;
      }
      Pack32( packet, 8, AcknowledgementNumber );
      packet[ 12 ] = 0x50;    // Header length and reserved
      packet[ 13 ] = flags;
      Pack16( packet, 14, CurrentWindow );
      Pack16( packet, 16, 0 );   // checksum placeholder
      Pack16( packet, 18, 0 );   // urgent pointer

      SequenceNumber += length;
      buffer->AcknowledgementNumber = SequenceNumber;

      while( (int32_t)(MaxSequenceTx - SequenceNumber) < 0 )
      {
         printf( "tx window full\n" );
         Event.Wait( __FILE__, __LINE__ );
      }

      checksum = ProtocolTCP::ComputeChecksum( packet, length+TCP_HEADER_SIZE, Config.IPv4.Address, RemoteAddress );

      Pack16( packet, 16, checksum );   // checksum

      buffer->Length += TCP_HEADER_SIZE;

      if( length > 0 || (flags & (FLAG_SYN|FLAG_FIN)) )
      {
         buffer->Disposable = false;
         buffer->Time_us = (uint32_t)osTime::GetTime();
         HoldingQueueLock.Take( __FILE__, __LINE__ );
         HoldingQueue.Put( buffer );
         HoldingQueueLock.Give();
      }

      ProtocolIP::Transmit( buffer, 0x06, RemoteAddress, Config.IPv4.Address );
   }
}

//============================================================================
//
//============================================================================

DataBuffer* ProtocolTCP::Connection::GetTxBuffer()
{
   DataBuffer* rc;

   rc = ProtocolIP::GetTxBuffer();
   if( rc )
   {
      rc->Packet    += TCP_HEADER_SIZE;
      rc->Remainder -= TCP_HEADER_SIZE;
   }

   return rc;
}

//============================================================================
//
//============================================================================

void ProtocolTCP::Connection::Write( const uint8_t *data, uint16_t length )
{
   uint16_t i;

   while( length > 0 )
   {
      if( !TxBuffer )
      {
         TxBuffer = GetTxBuffer();
         TxOffset = 0;
      }

      if( TxBuffer )
      {
         if( TxBuffer->Remainder > length )
         {
            for( i=0; i<length; i++ )
            {
               TxBuffer->Packet[ TxOffset++ ] = data[ i ];
            }
            TxBuffer->Length += length;
            TxBuffer->Remainder -= length;
            break;
         }
         else if( TxBuffer->Remainder <= length )
         {
            for( i=0; i<TxBuffer->Remainder; i++ )
            {
               TxBuffer->Packet[ TxOffset++ ] = data[ i ];
            }
            TxBuffer->Length += TxBuffer->Remainder;
            length -= TxBuffer->Remainder;
            data += TxBuffer->Remainder;
            TxBuffer->Remainder = 0;
            Flush();
         }
      }
      else
      {
         printf( "Out of tx buffers\n" );
      }
   }
}

//============================================================================
//
//============================================================================

void ProtocolTCP::Connection::Flush()
{
   if( TxBuffer != 0 )
   {
      BuildPacket( TxBuffer, FLAG_PSH );
      TxBuffer = 0;
   }
}

//============================================================================
//
//============================================================================

void ProtocolTCP::Connection::Close()
{
   switch( State )
   {
   case LISTEN:
   case SYN_SENT:
      State = CLOSED;
      break;
   case SYN_RECEIVED:
   case ESTABLISHED:
      SendFlags( FLAG_FIN );
      State = FIN_WAIT_1;
      SequenceNumber++;    // FIN consumes a sequence number
      break;
   case CLOSE_WAIT:
      SendFlags( FLAG_FIN );
      SequenceNumber++;    // FIN consumes a sequence number
      State = LAST_ACK;
      break;
   default:
      break;
   }
}

//============================================================================
//
//============================================================================

ProtocolTCP::Connection* ProtocolTCP::Connection::Listen()
{
   Connection* connection;

   while( NewConnection == 0 )
   {
      Event.Wait( __FILE__, __LINE__ );
   }
   connection = NewConnection;
   NewConnection = 0;

   return connection;
}

//============================================================================
//
//============================================================================

int ProtocolTCP::Connection::Read()
{
   int rc = -1;

   while( RxBufferEmpty )
   {
      if( LastAck != AcknowledgementNumber )
      {
         SendFlags( FLAG_ACK );
      }
      Event.Wait( __FILE__, __LINE__ );
   }

   rc = RxBuffer[ RxOutOffset++ ];
   if( RxOutOffset >= TCP_RX_WINDOW_SIZE )
   {
      RxOutOffset = 0;
   }
   AcknowledgementNumber++;
   CurrentWindow++;

   if( RxOutOffset == RxInOffset )
   {
      RxBufferEmpty = true;
   }

   //if( CurrentWindow == TCP_RX_WINDOW_SIZE && LastAck != AcknowledgementNumber )
   //{
   //   // The Rx buffer is empty, might as well ack
   //   Send( FLAG_ACK );
   //}

   return rc;
}

//============================================================================
//
//============================================================================

int ProtocolTCP::Connection::ReadLine( char* buffer, int size  )
{
   int      i;
   char     c;
   bool     done = false;
   int      bytesProcessed = 0;

   while( !done )
   {
      i = Read();
      if( i == -1 )
      {
         *buffer = 0;
         break;
      }
      c = (char)i;
      bytesProcessed++;
      switch( c )
      {
      case '\r':
         *buffer++ = 0;
         break;
      case '\n':
         *buffer++ = 0;
         done = true;
         break;
      default:
         *buffer++ = c;
         break;
      }

      if( bytesProcessed == size )
      {
         break;
      }
   }

   *buffer = 0;
   return bytesProcessed;
}

//============================================================================
//
//============================================================================

void ProtocolTCP::Connection::Tick()
{
   int count;
   int i;
   DataBuffer* buffer;
   uint32_t currentTime_us;
   uint32_t timeoutTime_us;

   HoldingQueueLock.Take( __FILE__, __LINE__ );
   count = HoldingQueue.GetCount();
   currentTime_us = (int32_t)osTime::GetTime();

   // Check for retransmit timeout
   timeoutTime_us = currentTime_us - TCP_RETRANSMIT_TIMEOUT_US;
   for( i=0; i<count; i++ )
   {
      buffer = (DataBuffer*)HoldingQueue.Get();
      if( (int32_t)(buffer->Time_us - timeoutTime_us) <= 0 )
      {
         printf( "TCP retransmit timeout %d, %d\n", buffer->Time_us, timeoutTime_us );
         buffer->Time_us = currentTime_us;
         ProtocolIP::Retransmit( buffer );
      }

      HoldingQueue.Put( buffer );
   }
   HoldingQueueLock.Give();

   // Check for TIMED_WAIT timeouts
   for( i=0; i<TCP_MAX_CONNECTIONS; i++ )
   {
      if( ConnectionList[ i ].State == TIMED_WAIT )
      {
         if( currentTime_us - ConnectionList[ i ].Time_us >= TCP_TIMED_WAIT_TIMEOUT_US  )
         {
            ConnectionList[ i ].State = CLOSED;
         }
      }
   }

   // Check for delayed ACK
   if( LastAck != AcknowledgementNumber )
   {
      SendFlags( FLAG_ACK );
   }
}

//============================================================================
//
//============================================================================

void ProtocolTCP::Connection::CalculateRTT( int32_t M )
{
   int32_t err;

   err = M - RTT_us;

   // Gain is 0.125
   RTT_us = RTT_us + (125 * err) / 1000;

   if( err < 0 )
   {
      err = -err;
   }

   // Gain is 0.250
   RTTDeviation = RTTDeviation + (250 * (err - RTTDeviation)) / 1000;
}

//============================================================================
//
//============================================================================

void ProtocolTCP::Connection::StoreRxData( DataBuffer* buffer )
{
   uint16_t i;

   if( buffer->Length > CurrentWindow )
   {
      printf( "Rx window overrun\n" );
      return;
   }

   for( i=0; i<buffer->Length; i++ )
   {
      RxBuffer[ RxInOffset++ ] = buffer->Packet[ i ];
      if( RxInOffset >= TCP_RX_WINDOW_SIZE )
      {
         RxInOffset = 0;
      }
   }
   CurrentWindow -= buffer->Length;
   RxBufferEmpty = false;
}

//============================================================================
//
//============================================================================

void ProtocolTCP::ProcessRx( DataBuffer* rxBuffer, const uint8_t* sourceIP, const uint8_t* targetIP )
{
   Connection* connection;
   uint16_t checksum;
   uint16_t localPort;
   uint16_t remotePort;
   uint8_t headerLength;
   uint8_t* data;
   uint16_t dataLength;
   int count;
   DataBuffer* buffer;
   uint8_t flags = 0;
   uint8_t* packet = rxBuffer->Packet;
   uint16_t length = rxBuffer->Length;
   uint16_t remoteWindowSize;
   uint32_t time_us;

   uint32_t SequenceNumber;
   uint32_t AcknowledgementNumber;

   checksum = ComputeChecksum( packet, length, sourceIP, targetIP );

   if( checksum == 0 )
   {
      // pass
      remotePort            = Unpack16( packet, 0 );
      localPort             = Unpack16( packet, 2 );
      SequenceNumber        = Unpack32( packet, 4 );
      AcknowledgementNumber = Unpack32( packet, 8 );
      headerLength          = (Unpack8( packet, 12 ) >> 4) * 4;
      remoteWindowSize      = Unpack16( packet, 14 );

      rxBuffer->Packet += headerLength;
      rxBuffer->Length -= headerLength;

      connection = LocateConnection( remotePort, sourceIP, localPort );
      if( connection == 0 )
      {
         // No connection found
         printf( "Connection port %d not found\n", localPort );
      }
      else
      {
         // Existing connection, process the state machine
         switch( connection->State )
         {
         case Connection::CLOSED:
            // Do nothing
            Reset( localPort, remotePort, sourceIP );
            break;
         case Connection::LISTEN:
            if( SYN )
            {
               // Need a closed connection to work with
               Connection* tmp = NewClient( sourceIP, remotePort, localPort );
               if( tmp != 0 )
               {
                  tmp->Parent = connection;
                  connection = tmp;
                  connection->State = Connection::SYN_RECEIVED;
                  connection->AcknowledgementNumber = SequenceNumber;
                  connection->LastAck = connection->AcknowledgementNumber;
                  connection->AcknowledgementNumber++;   // SYN flag consumes a sequence number
                  connection->SendFlags( FLAG_SYN | FLAG_ACK );
                  connection->SequenceNumber++; // Our SYN costs too
               }
               else
               {
                  printf( "Failed to get connection for SYN\n" );
               }
            }
            break;
         case Connection::SYN_SENT:
            if( SYN )
            {
               connection->AcknowledgementNumber = SequenceNumber;
               connection->LastAck = connection->AcknowledgementNumber;
               if( ACK )
               {
                  connection->State = Connection::ESTABLISHED;
                  connection->SendFlags( FLAG_ACK );
               }
               else
               {
                  // Simultaneous open
                  connection->State = Connection::SYN_RECEIVED;
                  connection->AcknowledgementNumber++;   // SYN flag consumes a sequence number
                  connection->SendFlags( FLAG_SYN | FLAG_ACK );
               }
            }
            break;
         case Connection::SYN_RECEIVED:
            if( ACK )
            {
               connection->State = Connection::ESTABLISHED;

               if( connection->Parent->NewConnection == 0 )
               {
                  connection->MaxSequenceTx = AcknowledgementNumber + remoteWindowSize;
                  connection->Parent->NewConnection = connection;
                  connection->Parent->Event.Notify();
               }
            }
            break;
         case Connection::ESTABLISHED:
            if( FIN )
            {
               connection->State = Connection::CLOSE_WAIT;
               connection->AcknowledgementNumber++;      // FIN consumes sequence number
               connection->SendFlags( FLAG_ACK );
            }
            break;
         case Connection::FIN_WAIT_1:
            if( FIN )
            {
               if( ACK )
               {
                  connection->State = Connection::TIMED_WAIT;
                  // Start TimedWait timer
               }
               else
               {
                  connection->State = Connection::CLOSING;
               }
               connection->AcknowledgementNumber++;      // FIN consumes sequence number
               connection->SendFlags( FLAG_ACK );
            }
            else if( ACK )
            {
               connection->State = Connection::FIN_WAIT_2;
            }
            break;
         case Connection::FIN_WAIT_2:
            if( FIN )
            {
               connection->State = Connection::TIMED_WAIT;
               // Start TimedWait timer
               connection->AcknowledgementNumber++;      // FIN consumes sequence number
               connection->Time_us = (int32_t)osTime::GetTime();
               connection->SendFlags( FLAG_ACK );
            }
            break;
         case Connection::CLOSE_WAIT:
            break;
         case Connection::CLOSING:
            break;
         case Connection::LAST_ACK:
            if( ACK )
            {
               connection->State = Connection::CLOSED;
            }
            break;
         case Connection::TIMED_WAIT:
            break;
         default:
            break;
         }

         // Handle any data received
         if
         (
            connection &&
            connection->State == Connection::ESTABLISHED ||
            connection->State == Connection::FIN_WAIT_1 ||
            connection->State == Connection::FIN_WAIT_2 ||
            connection->State == Connection::CLOSE_WAIT
         )
         {
            data = rxBuffer->Packet;
            dataLength = rxBuffer->Length;

            connection->MaxSequenceTx = AcknowledgementNumber + remoteWindowSize;
            connection->Event.Notify();

            // Handle any ACKed data
            if( ACK )
            {
               connection->HoldingQueueLock.Take( __FILE__, __LINE__ );
               count = connection->HoldingQueue.GetCount();
               time_us = (uint32_t)osTime::GetTime();
               for( int i=0; i<count; i++ )
               {
                  buffer = (DataBuffer*)connection->HoldingQueue.Get();
                  if( (int32_t)(AcknowledgementNumber - buffer->AcknowledgementNumber) >= 0 )
                  {
                     connection->CalculateRTT( (int32_t)(time_us - buffer->Time_us) );
                     ProtocolMACEthernet::FreeTxBuffer( buffer );
                  }
                  else
                  {
                     connection->HoldingQueue.Put( buffer );
                  }
               }
               connection->HoldingQueueLock.Give();
            }

            if( FIN )
            {
               if( connection->State == Connection::FIN_WAIT_1 )
               {
                  flags |= FLAG_ACK;
                  connection->State = Connection::CLOSE_WAIT;
               }
               else if( connection->State == Connection::ESTABLISHED )
               {
                  connection->State = Connection::CLOSE_WAIT;
                  flags |= FLAG_ACK;
               }
            }

            // ACK the receipt of the data
            if( dataLength > 0 )
            {
               // Copy it to the application
               rxBuffer->Disposable = false;
               connection->StoreRxData( rxBuffer );
               ProtocolMACEthernet::FreeRxBuffer( rxBuffer );
               connection->Event.Notify();
            }

            if( flags != 0 )
            {
               connection->SendFlags( flags );
            }
         }
      }
   }
   else
   {
      printf( "TCP Checksum Failure\n" );
   }
}

//============================================================================
//
//============================================================================

void ProtocolTCP::Reset( uint16_t localPort, uint16_t remotePort, const uint8_t* remoteAddress )
{
   uint8_t* packet;
   uint16_t checksum;
   uint16_t length;

   DataBuffer* buffer = ProtocolIP::GetTxBuffer();

   if( buffer == 0 )
   {
      return;
   }

   buffer->Packet    += TCP_HEADER_SIZE;
   buffer->Remainder -= TCP_HEADER_SIZE;

   buffer->Packet -= TCP_HEADER_SIZE;
   packet = buffer->Packet;
   length = buffer->Length;
   if( packet != 0 )
   {
      Pack16( packet, 0, localPort );
      Pack16( packet, 2, remotePort );
      Pack32( packet, 4, 0 );       // Sequence
      Pack32( packet, 8, 0 );       // AckSequence
      Pack8( packet, 12, 0x50 );    // Header length and reserved
      Pack8( packet, 13, FLAG_RST );
      Pack16( packet, 14, 0 );      // window size
      Pack16( packet, 16, 0 );      // clear checksum
      Pack16( packet, 18, 0 );      // 2 bytes of UrgentPointer

      checksum = ProtocolTCP::ComputeChecksum( packet, TCP_HEADER_SIZE, Config.IPv4.Address, remoteAddress );

      Pack16( packet, 16, checksum ); // checksum

      buffer->Length += TCP_HEADER_SIZE;
      buffer->Remainder -= buffer->Length;

      ProtocolIP::Transmit( buffer, 0x06, remoteAddress, Config.IPv4.Address );
   }
}

//============================================================================
//
//============================================================================

uint16_t ProtocolTCP::ComputeChecksum( uint8_t* packet, uint16_t length, const uint8_t* sourceIP, const uint8_t* targetIP )
{
   uint32_t checksum;
   uint16_t tmp;

   // A whole lot o' hokum just to compute the checksum
   checksum = FCS::ChecksumAdd( sourceIP, 4, 0 );
   checksum = FCS::ChecksumAdd( targetIP, 4, checksum );
   checksum += 0x06;    // protocol
   checksum += length;
   if( (length & 0x0001) != 0 )
   {
      // length is odd
      tmp = length+1;
      packet[ length ] = 0;
   }
   else
   {
      tmp = length;
   }
   checksum = FCS::ChecksumAdd( packet, tmp, checksum );

   return FCS::ChecksumComplete( checksum );
}

//============================================================================
//
//============================================================================

const char* ProtocolTCP::Connection::GetStateString()
{
   const char* rc;
   switch( State )
   {
   case ProtocolTCP::Connection::CLOSED:
      rc = "CLOSED";
      break;
   case ProtocolTCP::Connection::LISTEN:
      rc = "LISTEN";
      break;
   case ProtocolTCP::Connection::SYN_SENT:
      rc = "SYN_SENT";
      break;
   case ProtocolTCP::Connection::SYN_RECEIVED:
      rc = "SYN_RECEIVED";
      break;
   case ProtocolTCP::Connection::ESTABLISHED:
      rc = "ESTABLISHED";
      break;
   case ProtocolTCP::Connection::FIN_WAIT_1:
      rc = "FIN_WAIT_1";
      break;
   case ProtocolTCP::Connection::FIN_WAIT_2:
      rc = "FIN_WAIT_2";
      break;
   case ProtocolTCP::Connection::CLOSE_WAIT:
      rc = "CLOSE_WAIT";
      break;
   case ProtocolTCP::Connection::CLOSING:
      rc = "CLOSING";
      break;
   case ProtocolTCP::Connection::LAST_ACK:
      rc = "LAST_ACK";
      break;
   case ProtocolTCP::Connection::TIMED_WAIT:
      rc = "TIMED_WAIT";
      break;
   case ProtocolTCP::Connection::TTCP_PERSIST:
      rc = "TTCP_PERSIST";
      break;
   }
   return rc;
}

ProtocolTCP::Connection* ProtocolTCP::LocateConnection
(
   uint16_t remotePort,
   const uint8_t* remoteAddress,
   uint16_t localPort
)
{
   int i;

   // Must do two passes:
   // First pass to look for established connections
   // Second pass to look for listening connections

   // Pass 1
   for( i=0; i<TCP_MAX_CONNECTIONS; i++ )
   {
      //printf( "%s %d: connection port %d, state %s\n", FindFileName( __FILE__ ), __LINE__, ConnectionList[ i ].LocalPort, ConnectionList[ i ].GetStateString() );
      if
      (
         ConnectionList[ i ].LocalPort == localPort &&
         ConnectionList[ i ].RemotePort == remotePort &&
         Address::Compare( ConnectionList[ i ].RemoteAddress, remoteAddress, AddressConfiguration::IPv4AddressSize )
      )
      {
         return &ConnectionList[ i ];
      }
   }

   // Pass 2
   for( i=0; i<TCP_MAX_CONNECTIONS; i++ )
   {
      if( ConnectionList[ i ].LocalPort == localPort && ConnectionList[ i ].State == Connection::LISTEN )
      {
         return &ConnectionList[ i ];
      }
   }

   return 0;
}

//============================================================================
//
//============================================================================

uint16_t ProtocolTCP::NewPort()
{
   int i;

   if( NextPort <= 1024 )
   {
      NextPort = 1024;
   }

   NextPort++;

   for( i=0; i<TCP_MAX_CONNECTIONS; i++ )
   {
      if( ConnectionList[ i ].LocalPort == NextPort )
      {
         NextPort++;
         i=-1;
      }
   }

   return NextPort;
}

//============================================================================
//
//============================================================================

ProtocolTCP::Connection* ProtocolTCP::NewClient
(
   const uint8_t* remoteAddress,
   uint16_t remotePort,
   uint16_t localPort
)
{
   int i;
   int j;

   for( i=0; i<TCP_MAX_CONNECTIONS; i++ )
   {
      if( ConnectionList[ i ].State == Connection::CLOSED )
      {
         ConnectionList[ i ].LocalPort = localPort;
         ConnectionList[ i ].SequenceNumber = 1;
         ConnectionList[ i ].MaxSequenceTx = ConnectionList[ i ].SequenceNumber + 1024;
         for( j=0; j<AddressConfiguration::IPv4AddressSize; j++ )
         {
            ConnectionList[ i ].RemoteAddress[ j ] = remoteAddress[ j ];
         }
         ConnectionList[ i ].RemotePort = remotePort;

         return &ConnectionList[ i ];
      }
   }

   return 0;
}

//============================================================================
//
//============================================================================

ProtocolTCP::Connection* ProtocolTCP::NewServer( uint16_t port )
{
   int i;

   for( i=0; i<TCP_MAX_CONNECTIONS; i++ )
   {
      if( ConnectionList[ i ].State == Connection::CLOSED )
      {
         ConnectionList[ i ].State = Connection::LISTEN;
         ConnectionList[ i ].LocalPort = port;
         return &ConnectionList[ i ];
      }
   }

   return 0;
}

//============================================================================
//
//============================================================================

void ProtocolTCP::Initialize()
{
}

//============================================================================
//
//============================================================================

void ProtocolTCP::Tick()
{
   int i;

   for( i=0; i<TCP_MAX_CONNECTIONS; i++ )
   {
      if
      (
         ConnectionList[ i ].State == Connection::ESTABLISHED ||
         ConnectionList[ i ].State == Connection::TIMED_WAIT
      )
      {
         ConnectionList[ i ].Tick();
      }
   }
}

void ProtocolTCP::Show( osPrintfInterface* out )
{
   out->Printf( "TCP Information\n" );
   for( int i = 0; i<TCP_MAX_CONNECTIONS; i++ )
   {
      out->Printf( "connection %s   ", ConnectionList[ i ].GetStateString() );
      switch( ConnectionList[ i ].State )
      {
      case Connection::LISTEN:
         out->Printf( "     local=%d  ", ConnectionList[ i ].LocalPort );
         break;
      case Connection::ESTABLISHED:
         out->Printf( "local=%d  remote=%d.%d.%d.%d:%d", ConnectionList[ i ].LocalPort, ConnectionList[ i ].RemoteAddress[0], ConnectionList[ i ].RemoteAddress[ 1 ], ConnectionList[ i ].RemoteAddress[ 2 ], ConnectionList[ i ].RemoteAddress[ 3 ], ConnectionList[ i ].RemotePort );
         break;
      default:
         break;
      }
      out->Printf( "\n" );
   }
}
