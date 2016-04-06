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

#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <inttypes.h>
#include "osEvent.h"
#include "osQueue.h"
#include "osMutex.h"
#include "Config.h"
#include "ProtocolIPv4.h"

class DataBuffer;

class TCPConnection
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
   uint8_t  RemoteAddress[ ProtocolIPv4::AddressSize ];
   uint32_t SequenceNumber;
   uint32_t AcknowledgementNumber;
   uint32_t LastAck;
   uint32_t MaxSequenceTx;
   uint32_t RTT_us;
   uint32_t RTTDeviation;
   uint32_t Time_us;

   ~TCPConnection();
   void SendFlags( uint8_t flags );
   void Close();
   TCPConnection* Listen();

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
   TCPConnection* NewConnection;
   TCPConnection* Parent;

   osEvent Event;
   osQueue HoldingQueue;
   osMutex HoldingQueueLock;
   void* ConnectionHoldingBuffer[ TX_BUFFER_COUNT ];

   // Bad, bad, bad!
   // These should be references, but connections are stored in an array and thus
   // needs a default constructor, so can't pass in these references.
   // This will need work, but OK for now.
   ProtocolIPv4* IP;
   ProtocolTCP* TCP;

   void Tick();

   TCPConnection();
   void Initialize( ProtocolIPv4&, ProtocolTCP& );
   TCPConnection( TCPConnection& );
};

#endif

