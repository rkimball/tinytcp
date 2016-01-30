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

// DHCP Info https://en.wikipedia.org/wiki/Dynamic_Host_Configuration_Protocol
// DHCP Options http://www.faqs.org/rfcs/rfc1533.html

#include <stdio.h>
#include <string.h>
#include "ProtocolDHCP.h"
#include "ProtocolUDP.h"
#include "ProtocolIP.h"
#include "NetworkInterface.h"
#include "Utility.h"
#include "osTime.h"

extern AddressConfiguration Config;
int32_t ProtocolDHCP::PendingXID = -1;
static const uint32_t DHCP_MAGIC = 0x63825363;

void ProtocolDHCP::test()
{
   printf( "sending discover\n" );
   Discover();
   printf( "discover sent\n" );
}

void ProtocolDHCP::ProcessRx( DataBuffer* buffer )
{
   uint8_t op = Unpack8( buffer->Packet, 0 );
   uint8_t htype = Unpack8( buffer->Packet, 1 );
   uint8_t hlen = Unpack8( buffer->Packet, 2 );
   uint8_t hops = Unpack8( buffer->Packet, 3 );
   uint32_t xid = Unpack32( buffer->Packet, 4 );
   uint16_t secs = Unpack16( buffer->Packet, 8 );
   uint16_t flags = Unpack16( buffer->Packet, 10 );
   uint32_t ciaddr = Unpack32( buffer->Packet, 12 ); // (Client IP address)
   uint32_t yiaddr = Unpack32( buffer->Packet, 16 ); // (Your IP address)
   uint32_t siaddr = Unpack32( buffer->Packet, 20 ); // (Server IP address)
   uint32_t giaddr = Unpack32( buffer->Packet, 24 ); // (Gateway IP address)
   uint32_t magic = Unpack32( buffer->Packet, 236 );

   printf( "op = %d\n", op );
   printf( "htype = %d\n", htype );
   printf( "hlen = %d\n", hlen );
   printf( "hops = %d\n", hops );
   printf( "xid = 0x%0X\n", xid );
   printf( "secs = %d\n", secs );
   printf( "flags = %d\n", flags );
   printf( "ciaddr = %s\n", inet_ntoa( ciaddr ) ); // (Client IP address)
   printf( "yiaddr = %s\n", inet_ntoa( yiaddr ) ); // (Your IP address)
   printf( "siaddr = %s\n", inet_ntoa( siaddr ) ); // (Server IP address)
   printf( "giaddr = %s\n", inet_ntoa( giaddr ) ); // (Gateway IP address)
   if( xid == PendingXID )
   {
      printf( "*********** this is for me ************\n" );
   }
   printf( "magic = 0x%0X\n", magic );
}

//9.4.DHCP Message Type
//
//This option is used to convey the type of the DHCP message.The code
//for this option is 53, and its length is 1.  Legal values for this
//option are :
//
//Value   Message Type
//---- - ------------
//1     DHCPDISCOVER
//2     DHCPOFFER
//3     DHCPREQUEST
//4     DHCPDECLINE
//5     DHCPACK
//6     DHCPNAK
//7     DHCPRELEASE
//
//Code   Len  Type
//+ ---- - +---- - +---- - +
//| 53 | 1 | 1 - 7 |
//+---- - +---- - +---- - +

void ProtocolDHCP::Discover()
{
   DataBuffer* buffer = ProtocolUDP::GetTxBuffer();
   int i;

   if( buffer )
   {
      PendingXID = (uint32_t)osTime::GetProcessorTime();

      buffer->Length = Pack8( buffer->Packet, buffer->Length, 1 );   // op
      buffer->Length = Pack8( buffer->Packet, buffer->Length, 1 );   // htype
      buffer->Length = Pack8( buffer->Packet, buffer->Length, 6 );   // hlen
      buffer->Length = Pack8( buffer->Packet, buffer->Length, 0 );   // hops
      buffer->Length = Pack32( buffer->Packet, buffer->Length, PendingXID );     // xid
      buffer->Length = Pack16( buffer->Packet, buffer->Length, 0 );     // secs
      buffer->Length = Pack16( buffer->Packet, buffer->Length, 0x8000 );  // flags
      buffer->Length = Pack32( buffer->Packet, buffer->Length, 0 ); // (Client IP address)
      buffer->Length = Pack32( buffer->Packet, buffer->Length, 0 ); // (Your IP address)
      buffer->Length = Pack32( buffer->Packet, buffer->Length, 0 ); // (Server IP address)
      buffer->Length = Pack32( buffer->Packet, buffer->Length, 0 ); // (Gateway IP address)
      for( i = 0; i < 6; i++ ) buffer->Packet[ buffer->Length++ ] = Config.Address.Hardware[ i ];
      for( ; i < 16; i++ ) buffer->Packet[ buffer->Length++ ] = 0;   // pad chaddr to 16 bytes
      for( i = 0; i < 64; i++ ) buffer->Packet[ buffer->Length++ ] = 0; // sname
      for( i = 0; i < 128; i++ ) buffer->Packet[ buffer->Length++ ] = 0; // file
      buffer->Length = Pack32( buffer->Packet, buffer->Length, DHCP_MAGIC );

      // Add options
      buffer->Packet[ buffer->Length++ ] = 53;
      buffer->Packet[ buffer->Length++ ] = 1;
      buffer->Packet[ buffer->Length++ ] = 1; // DHCP Discover

      // client id
      buffer->Packet[ buffer->Length++ ] = 61;
      buffer->Packet[ buffer->Length++ ] = 7; // length
      buffer->Packet[ buffer->Length++ ] = 1; // type is hardware address
      for( int i = 0; i < 6; i++ ) buffer->Packet[ buffer->Length++ ] = Config.Address.Hardware[ i ];

      // host name
      const char* name = "tinytcp";
      buffer->Packet[ buffer->Length++ ] = 12;
      buffer->Packet[ buffer->Length++ ] = strlen( name );
      for( int i = 0; i < strlen( name ); i++ ) buffer->Packet[ buffer->Length++ ] = name[ i ];

      // parameter request list
      buffer->Packet[ buffer->Length++ ] = 55;
      buffer->Packet[ buffer->Length++ ] = 4; // length
      buffer->Packet[ buffer->Length++ ] = 1; // subnet mask
      buffer->Packet[ buffer->Length++ ] = 3; // router
      buffer->Packet[ buffer->Length++ ] = 6; // dns
      buffer->Packet[ buffer->Length++ ] = 15; // domain name

      buffer->Packet[ buffer->Length++ ] = 255;  // End options

      int pad = 8;
      for( int i = 0; i < pad; i++ ) buffer->Packet[ buffer->Length++ ] = 0;

      uint8_t sourceIP[] = { 0, 0, 0, 0 };
      uint8_t targetIP[] = { 255, 255, 255, 255 };
      ProtocolUDP::Transmit( buffer, targetIP, 67, sourceIP, 68 );
   }
}
