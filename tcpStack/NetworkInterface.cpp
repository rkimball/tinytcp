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

#include "NetworkInterface.h"
#include "ProtocolMACEthernet.h"

extern AddressConfiguration Config;

void NetworkInterface::RxData( void* data, size_t length )
{
   ProtocolMACEthernet::ProcessRx( (uint8_t*)data, length );
}

void NetworkRxData( void* data, size_t length )
{
   ProtocolMACEthernet::ProcessRx( (uint8_t*)data, length );
}

void AddressConfiguration::Show( osPrintfInterface* out )
{
   out->Printf( "Network Configuration\n" );
   out->Printf( "Ethernet MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
      MACAddress[ 0 ], MACAddress[ 1 ], MACAddress[ 2 ],
      MACAddress[ 3 ], MACAddress[ 4 ], MACAddress[ 5 ] );
   //uint8_t BroadcastMACAddress[ AddressConfiguration::MACAddressSize ];

   out->Printf( "IPv4 Configuration\n" );
   out->Printf( "   Address:          %d.%d.%d.%d\n", IPv4.Address[ 0 ], IPv4.Address[ 1 ], IPv4.Address[ 2 ], IPv4.Address[ 3 ] );
   out->Printf( "   SubnetMask:       %d.%d.%d.%d\n", IPv4.SubnetMask[ 0 ], IPv4.SubnetMask[ 1 ], IPv4.SubnetMask[ 2 ], IPv4.SubnetMask[ 3 ] );
   out->Printf( "   Gateway:          %d.%d.%d.%d\n", IPv4.Gateway[ 0 ], IPv4.Gateway[ 1 ], IPv4.Gateway[ 2 ], IPv4.Gateway[ 3 ] );
   out->Printf( "   DomainNameServer: %d.%d.%d.%d\n", IPv4.DomainNameServer[ 0 ], IPv4.DomainNameServer[ 1 ], IPv4.DomainNameServer[ 2 ], IPv4.DomainNameServer[ 3 ] );
   out->Printf( "   BroadcastAddress: %d.%d.%d.%d\n", IPv4.BroadcastAddress[ 0 ], IPv4.BroadcastAddress[ 1 ], IPv4.BroadcastAddress[ 2 ], IPv4.BroadcastAddress[ 3 ] );
   //   uint32_t IpAddressLeaseTime = 0;
   //   uint32_t RenewTime = 0;
   //   uint32_t RebindTime = 0;
   //   uint8_t SubnetMask[ IPv4AddressSize ];
   //   uint8_t Gateway[ IPv4AddressSize ];
   //   uint8_t DomainNameServer[ IPv4AddressSize ];
   //   uint8_t BroadcastAddress[ IPv4AddressSize ];
}

