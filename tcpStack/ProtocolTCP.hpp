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
#include "osQueue.h"
#include "osMutex.h"
#include "DataBuffer.h"
#include "osEvent.h"
#include "TCPConnection.h"

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

class ProtocolIPv4;

class ProtocolTCP
{
public:
   friend class TCPConnection;

   ProtocolTCP( ProtocolIPv4& );
   void Tick();

   TCPConnection* NewClient( InterfaceMAC*, const uint8_t* remoteAddress, uint16_t remotePort, uint16_t localPort );
   TCPConnection* NewServer( InterfaceMAC*, uint16_t port );
   uint16_t NewPort();

   void ProcessRx( DataBuffer*, const uint8_t* sourceIP, const uint8_t* targetIP );
   void Show( osPrintfInterface* out );

private:
   TCPConnection* LocateConnection( uint16_t remotePort, const uint8_t* remoteAddress, uint16_t localPort );
   static uint16_t ComputeChecksum( uint8_t* packet, uint16_t length, const uint8_t* sourceIP, const uint8_t* targetIP );
   void Reset( InterfaceMAC*, uint16_t localPort, uint16_t remotePort, const uint8_t* remoteAddress );

   TCPConnection ConnectionList[ TCP_MAX_CONNECTIONS ];
   void* ConnectionHoldingBuffer[ TX_BUFFER_COUNT ];
   uint16_t NextPort;

   ProtocolIPv4& IP;

   ProtocolTCP();
   ProtocolTCP( ProtocolTCP& );
};

#endif
