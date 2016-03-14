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

#ifndef PROTOCOLTCP_H
#define PROTOCOLTCP_H

#include <inttypes.h>
#include "ProtocolTCP.h"
#include "NetworkInterface.h"
#include "Address.h"
#include "osQueue.h"
#include "osMutex.h"
#include "DataBuffer.h"
#include "osEvent.h"

// SourcePort - 16 bits
// TargetPort - 16 bits
// Sequence - 32 bits
// AckSequence - 32 bits
// HeaderLength - 4 bits length in 32 bit words
// Reserved - 6 bits
// URG - 1 bit
// ACK - 1 bit
// PSH - 1 bit
// RST - 1 bit
// SYN - 1 bit
// FIN - 1 bit
// WindowSize - 16 bits
// Checksum - 16 bits
// UrgentPointer - 16 bits

#define TCP_HEADER_SIZE (20)
#define TCP_MAX_CONNECTIONS (5)
#define TCP_RX_WINDOW_SIZE (256)
#if TCP_RX_WINDOW_SIZE > (DATA_BUFFER_PAYLOAD_SIZE - TCP_HEADER_SIZE - IP_HEADER_SIZE - MAC_HEADER_SIZE)
#error Rx window size must be smaller than data payload
#endif
#define TCP_RETRANSMIT_TIMEOUT_US 100000
#define TCP_TIMED_WAIT_TIMEOUT_US 1000000

#define FLAG_URG (0x20)
#define FLAG_ACK (0x10)
#define FLAG_PSH (0x08)
#define FLAG_RST (0x04)
#define FLAG_SYN (0x02)
#define FLAG_FIN (0x01)

#define URG (packet[ 13 ] & FLAG_URG)
#define ACK (packet[ 13 ] & FLAG_ACK)
#define PSH (packet[ 13 ] & FLAG_PSH)
#define RST (packet[ 13 ] & FLAG_RST)
#define SYN (packet[ 13 ] & FLAG_SYN)
#define FIN (packet[ 13 ] & FLAG_FIN)

class ProtocolTCP
{
public:
   class Connection
   {
   public:
      typedef enum States
      {
         CLOSED = 0,
         LISTEN,
         SYN_SENT,
         SYN_RECEIVED,
         ESTABLISHED,
         FIN_WAIT_1,
         FIN_WAIT_2,
         CLOSE_WAIT,
         CLOSING,
         LAST_ACK,
         TIMED_WAIT,
         TTCP_PERSIST
      } TCP_STATES;

      friend class ProtocolTCP;

      States State;
      uint16_t LocalPort;
      uint16_t RemotePort;
      uint8_t  RemoteAddress[ AddressConfiguration::IPv4AddressSize ];
      uint32_t SequenceNumber;
      uint32_t AcknowledgementNumber;
      uint32_t LastAck;
      uint32_t MaxSequenceTx;
      uint32_t RTT_us;
      uint32_t RTTDeviation;
      uint32_t Time_us;

      ~Connection();
      void SendFlags( uint8_t flags );
      void Close();
      Connection* Listen();

      int Read();
      int ReadLine( char* buffer, int size );
      void Write( const uint8_t* data, uint16_t length );
      void Flush();
      const char* GetStateString();
   
   private:
      uint16_t RxInOffset;
      uint16_t RxOutOffset;
      uint16_t TxOffset;    // Offset into Data used by Write() method
      uint16_t CurrentWindow;

      DataBuffer* TxBuffer;
      uint8_t RxBuffer[ TCP_RX_WINDOW_SIZE ];
      bool RxBufferEmpty;
      void StoreRxData( DataBuffer* buffer );

      DataBuffer* GetTxBuffer();
      void BuildPacket( DataBuffer*, uint8_t flags );
      void CalculateRTT( int32_t msRTT );

      // This stuff is used for Listening for incomming connections
      Connection* NewConnection;
      Connection* Parent;

      osEvent Event;
      osQueue  HoldingQueue;
      osMutex  HoldingQueueLock;
      void Tick();
      
      Connection();
      Connection( Connection& );
   };

   friend class Connection;

   static void Initialize();
   static void Tick();

   static Connection* NewClient( const uint8_t* remoteAddress, uint16_t remotePort, uint16_t localPort );
   static Connection* NewServer( uint16_t port );
   static uint16_t NewPort();

   static void ProcessRx( DataBuffer*, const uint8_t* sourceIP, const uint8_t* targetIP );
   static void Show( osPrintfInterface* out );

private:
   static Connection* LocateConnection( uint16_t remotePort, const uint8_t* remoteAddress, uint16_t localPort );
   static uint16_t ComputeChecksum( uint8_t* packet, uint16_t length, const uint8_t* sourceIP, const uint8_t* targetIP );
   static void Reset( uint16_t localPort, uint16_t remotePort, const uint8_t* remoteAddress );

   static Connection ConnectionList[ TCP_MAX_CONNECTIONS ];
   static uint16_t NextPort;

   ProtocolTCP();
   ProtocolTCP( ProtocolTCP& );
};

#endif
