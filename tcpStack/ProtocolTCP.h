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

#ifndef PROTOCOLTCP_H
#define PROTOCOLTCP_H

#include <cinttypes>
#include "ProtocolTCP.h"
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
#define TCP_MAX_CONNECTIONS 5
#define TCP_RX_WINDOW_SIZE (256)
#if TCP_RX_WINDOW_SIZE > (DATA_BUFFER_PAYLOAD_SIZE-20-20-10)
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
      uint8_t  RemoteAddress[ Address::ProtocolSize ];
      int32_t SequenceNumber;
      int32_t AcknowledgementNumber;
      int32_t LastAck;
      int32_t MaxSequenceTx;
      int32_t RTT_us;
      int32_t RTTDeviation;
      int32_t Time_us;

      ~Connection();
      void Send( uint8_t flags );
      void Close();
      Connection* Listen();

      int Read();
      int ReadLine( char* buffer, int size );
      void Write( uint8_t* data, uint16_t length );
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
      osEvent Event;
      Connection* NewConnection;
      Connection* Parent;

      osQueue  HoldingQueue;
      osMutex  HoldingQueueLock;
      void Tick();
      
      Connection();
      Connection( Connection& );
   };

   friend class Connection;

   static void Initialize();
   static void Tick();

   static Connection* NewClient( uint8_t* remoteAddress, uint16_t remotePort, uint16_t localPort );
   static Connection* NewServer( uint16_t port );
   static uint16_t NewPort();

   static void ProcessRx( DataBuffer*, uint8_t* sourceIP, uint8_t* targetIP );
   static void Show( osPrintfInterface* out );

private:
   static Connection* LocateConnection( uint16_t remotePort, uint8_t* remoteAddress, uint16_t localPort );
   static uint16_t ComputeChecksum( uint8_t* packet, uint16_t length, uint8_t* sourceIP, uint8_t* targetIP );
   static void Reset( uint16_t localPort, uint16_t remotePort, uint8_t* remoteAddress );

   static Connection ConnectionList[ TCP_MAX_CONNECTIONS ];
   static uint16_t NextPort;

   ProtocolTCP();
   ProtocolTCP( ProtocolTCP& );
};

#endif
