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
#include <windows.h>

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

extern AddressConfiguration Config;

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

ProtocolTCP::Connection::~Connection()
{
}

//============================================================================
//
//============================================================================

void ProtocolTCP::Connection::Send( uint8_t flags )
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
      packet[  0 ] = LocalPort >> 8;
      packet[  1 ] = LocalPort & 0xFF;
      packet[  2 ] = RemotePort >> 8;
      packet[  3 ] = RemotePort & 0xFF;
      packet[  4 ] = SequenceNumber >> 24;
      packet[  5 ] = SequenceNumber >> 16;
      packet[  6 ] = SequenceNumber >>  8;
      packet[  7 ] = SequenceNumber >>  0;
      if( AcknowledgementNumber - LastAck > 0 )
      {
         // Only advance LastAck if Ack > LastAck
         LastAck = AcknowledgementNumber;
      }
      packet[  8 ] = AcknowledgementNumber >> 24;
      packet[  9 ] = AcknowledgementNumber >> 16;
      packet[ 10 ] = AcknowledgementNumber >>  8;
      packet[ 11 ] = AcknowledgementNumber >>  0;
      packet[ 12 ] = 0x50;    // Header length and reserved
      packet[ 13 ] = flags;
      packet[ 14 ] = CurrentWindow >> 8;
      packet[ 15 ] = (CurrentWindow & 0xFF);
      packet[ 16 ] = 0;    // 2 bytes of CheckSum
      packet[ 17 ] = 0;
      packet[ 18 ] = 0;    // 2 bytes of UrgentPointer
      packet[ 19 ] = 0;

      SequenceNumber += length;
      buffer->AcknowledgementNumber = SequenceNumber;

      while( MaxSequenceTx - SequenceNumber < 0 )
      {
         printf( "tx window full\n" );
         Event.Wait( __FILE__, __LINE__ );
      }

      checksum = ProtocolTCP::ComputeChecksum( packet, length+TCP_HEADER_SIZE, Config.IPv4.Address, RemoteAddress );

      packet[ 16 ] = checksum >> 8;
      packet[ 17 ] = checksum & 0xFF;

      buffer->Length += TCP_HEADER_SIZE;
      buffer->Remainder -= buffer->Length;

      if( length > 0 || (flags & (FLAG_SYN|FLAG_FIN)) )
      {
         buffer->Disposable = false;
         buffer->Time_us = (int32_t)osTime::GetTime();
         HoldingQueueLock.Take( __FILE__, __LINE__ );
         HoldingQueue.Put( buffer );
         HoldingQueueLock.Give();
      }

      ProtocolIP::Transmit( buffer, 0x06, RemoteAddress );
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

void ProtocolTCP::Connection::Write( uint8_t *data, uint16_t length )
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
      Send( FLAG_FIN );
      State = FIN_WAIT_1;
      SequenceNumber++;    // FIN consumes a sequence number
      break;
   case CLOSE_WAIT:
      Send( FLAG_FIN );
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
         Send( FLAG_ACK );
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
   uint8_t       done = 0;
   int      loop = 0;
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
         done = 1;
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
   int32_t currentTime_us;
   int32_t timeoutTime_us;

   HoldingQueueLock.Take( __FILE__, __LINE__ );
   count = HoldingQueue.GetCount();
   currentTime_us = (int32_t)osTime::GetTime();
   timeoutTime_us = currentTime_us - TCP_RETRANSMIT_TIMEOUT_US;
   for( i=0; i<count; i++ )
   {
      buffer = (DataBuffer*)HoldingQueue.Get();
      if( (buffer->Time_us - timeoutTime_us) <= 0 )
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
            printf( "TIMED_WAIT timeout, close connection\n" );
            ConnectionList[ i ].State = CLOSED;
         }
      }
   }

   // Check for delayed ACK
   if( LastAck != AcknowledgementNumber )
   {
      Send( FLAG_ACK );
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

void ProtocolTCP::ProcessRx( DataBuffer* rxBuffer, uint8_t* sourceIP, uint8_t* targetIP )
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
   uint16_t windowSize;
   int32_t time_us;

   int32_t SequenceNumber;
   int32_t AcknowledgementNumber;

   checksum = ComputeChecksum( packet, length, sourceIP, targetIP );

   if( checksum == 0 )
   {
      // pass
      remotePort = (packet[ 0 ] << 8) | packet[ 1 ];
      localPort    = (packet[ 2 ] << 8) | packet[ 3 ];
      headerLength = (packet[ 12 ] >> 4) * 4;
      windowSize = (packet[ 14 ] << 8) | packet[ 15 ];

      SequenceNumber  = packet[ 4 ] << 24;
      SequenceNumber |= packet[ 5 ] << 16;
      SequenceNumber |= packet[ 6 ] <<  8;
      SequenceNumber |= packet[ 7 ] <<  0;

      AcknowledgementNumber  = packet[  8 ] << 24;
      AcknowledgementNumber |= packet[  9 ] << 16;
      AcknowledgementNumber |= packet[ 10 ] <<  8;
      AcknowledgementNumber |= packet[ 11 ] <<  0;

      rxBuffer->Packet += headerLength;
      rxBuffer->Length -= headerLength;

      connection = LocateConnection( remotePort, sourceIP, localPort );
      if( connection == 0 )
      {
         // No connection found
         //printf( "Connection port %d not found\n", localPort );
      }
      else
      {
         // Existing connection, process the state machine
         switch( connection->State )
         {
         case Connection::CLOSED:
            // Do nothing
            printf( "CLOSED\n" );
            Reset( localPort, remotePort, sourceIP );
            break;
         case Connection::LISTEN:
            printf( "LISTEN\n" );
            if( SYN )
            {
               // Need a closed connection to work with
               Connection* tmp = NewClient( sourceIP, remotePort, localPort );
               if( tmp != 0 )
               {
                  tmp->Parent = connection;
                  connection = tmp;
                  connection->State = Connection::SYN_RECEIVED;
                  printf( "Enter SYN_RECEIVED\n" );
                  connection->AcknowledgementNumber = SequenceNumber;
                  connection->LastAck = connection->AcknowledgementNumber;
                  connection->AcknowledgementNumber++;   // SYN flag consumes a sequence number
                  connection->Send( FLAG_SYN | FLAG_ACK );
                  connection->SequenceNumber++; // Our SYN costs too
               }
               else
               {
                  printf( "Failed to get connection for SYN\n" );
               }
            }
            break;
         case Connection::SYN_SENT:
            printf( "SYN_SENT\n" );
            if( SYN )
            {
               connection->AcknowledgementNumber = SequenceNumber;
               connection->LastAck = connection->AcknowledgementNumber;
               if( ACK )
               {
                  connection->State = Connection::ESTABLISHED;
                  connection->Send( FLAG_ACK );
                  printf( "Enter ESTABLISHED\n" );
               }
               else
               {
                  // Simultaneous open
                  connection->State = Connection::SYN_RECEIVED;
                  connection->AcknowledgementNumber++;   // SYN flag consumes a sequence number
                  connection->Send( FLAG_SYN | FLAG_ACK );
                  printf( "Enter SYN_RECEIVED\n" );
               }
            }
            break;
         case Connection::SYN_RECEIVED:
            printf( "SYN_RECEIVED\n" );
            if( ACK )
            {
               connection->State = Connection::ESTABLISHED;
               printf( "Enter ESTABLISHED\n" );

               if( connection->Parent->NewConnection == 0 )
               {
                  connection->MaxSequenceTx = AcknowledgementNumber + windowSize;
                  connection->Parent->NewConnection = connection;
                  connection->Parent->Event.Notify();
               }
            }
            break;
         case Connection::ESTABLISHED:
            printf( "ESTABLISHED\n" );
            if( FIN )
            {
               connection->State = Connection::CLOSE_WAIT;
               connection->AcknowledgementNumber++;      // FIN consumes sequence number
               connection->Send( FLAG_ACK );
               printf( "Enter CLOSE_WAIT\n" );
            }
            break;
         case Connection::FIN_WAIT_1:
            printf( "FIN_WAIT_1\n" );
            if( FIN )
            {
               if( ACK )
               {
                  connection->State = Connection::TIMED_WAIT;
                  printf( "Enter TIMED_WAIT\n" );
                  // Start TimedWait timer
               }
               else
               {
                  connection->State = Connection::CLOSING;
                  printf( "Enter CLOSING\n" );
               }
               connection->AcknowledgementNumber++;      // FIN consumes sequence number
               connection->Send( FLAG_ACK );
            }
            else if( ACK )
            {
               connection->State = Connection::FIN_WAIT_2;
               printf( "Enter FIN_WAIT_2\n" );
            }
            break;
         case Connection::FIN_WAIT_2:
            printf( "FIN_WAIT_2\n" );
            if( FIN )
            {
               connection->State = Connection::TIMED_WAIT;
               printf( "Enter TIMED_WAIT\n" );
               // Start TimedWait timer
               connection->AcknowledgementNumber++;      // FIN consumes sequence number
               connection->Time_us = (int32_t)osTime::GetTime();
               connection->Send( FLAG_ACK );
            }
            break;
         case Connection::CLOSE_WAIT:
            printf( "CLOSE_WAIT\n" );
            break;
         case Connection::CLOSING:
            printf( "CLOSING\n" );
            break;
         case Connection::LAST_ACK:
            printf( "LAST_ACK\n" );
            if( ACK )
            {
               connection->State = Connection::CLOSED;
               printf( "Enter CLOSED\n" );
            }
            break;
         case Connection::TIMED_WAIT:
            //printf( "TIMED_WAIT\n" );
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

            connection->MaxSequenceTx = AcknowledgementNumber + windowSize;
            connection->Event.Notify();
            //printf( "MaxSequenceTx set to %d\n", connection->MaxSequenceTx );

            // Handle any ACKed data
            if( ACK )
            {
               //printf( "ACK %d\n", AcknowledgementNumber );
               connection->HoldingQueueLock.Take( __FILE__, __LINE__ );
               count = connection->HoldingQueue.GetCount();
               time_us = (int32_t)osTime::GetTime();
               for( int i=0; i<count; i++ )
               {
                  buffer = (DataBuffer*)connection->HoldingQueue.Get();
                  if( AcknowledgementNumber - buffer->AcknowledgementNumber >= 0 )
                  {
                     connection->CalculateRTT( time_us - buffer->Time_us );
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
               connection->Send( flags );
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

void ProtocolTCP::Reset( uint16_t localPort, uint16_t remotePort, uint8_t* remoteAddress )
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
      packet[  0 ] = localPort >> 8;
      packet[  1 ] = localPort & 0xFF;
      packet[  2 ] = remotePort >> 8;
      packet[  3 ] = remotePort & 0xFF;
      packet[  4 ] = 0;
      packet[  5 ] = 0;
      packet[  6 ] = 0;
      packet[  7 ] = 0;
      packet[  8 ] = 0;
      packet[  9 ] = 0;
      packet[ 10 ] = 0;
      packet[ 11 ] = 0;
      packet[ 12 ] = 0x50;    // Header length and reserved
      packet[ 13 ] = FLAG_RST;
      packet[ 14 ] = 0;
      packet[ 15 ] = 0;
      packet[ 16 ] = 0;    // 2 bytes of CheckSum
      packet[ 17 ] = 0;
      packet[ 18 ] = 0;    // 2 bytes of UrgentPointer
      packet[ 19 ] = 0;

      checksum = ProtocolTCP::ComputeChecksum( packet, TCP_HEADER_SIZE, Config.IPv4.Address, remoteAddress );

      packet[ 16 ] = checksum >> 8;
      packet[ 17 ] = checksum & 0xFF;

      buffer->Length += TCP_HEADER_SIZE;
      buffer->Remainder -= buffer->Length;

      ProtocolIP::Transmit( buffer, 0x06, remoteAddress );
   }
}

//============================================================================
//
//============================================================================

uint16_t ProtocolTCP::ComputeChecksum( uint8_t* packet, uint16_t length, uint8_t* sourceIP, uint8_t* targetIP )
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
   char* rc;
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
   uint8_t* remoteAddress,
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
   uint8_t* remoteAddress,
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
