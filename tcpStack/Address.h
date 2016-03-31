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

#ifndef ADDRESS_H
#define ADDRESS_H

#include <inttypes.h>
#include <stddef.h>
#include <osPrintfInterface.h>
#include "Config.h"

class Address
{
public:

   static bool Compare( const uint8_t* a1, const uint8_t* a2, int length );
};

class AddressConfiguration
{
public:
   //Address Address;

   class IPv4_t
   {
   public:
      IPv4_t();

      bool  DataValid;
      uint8_t Address[ IPv4AddressSize ];
      uint32_t IpAddressLeaseTime;
      uint32_t RenewTime;
      uint32_t RebindTime;
      uint8_t SubnetMask[ IPv4AddressSize ];
      uint8_t Gateway[ IPv4AddressSize ];
      uint8_t DomainNameServer[ IPv4AddressSize ];
      uint8_t BroadcastAddress[ IPv4AddressSize ];
   } IPv4;

   void Show( osPrintfInterface* out );
};

extern AddressConfiguration Config;

#endif
