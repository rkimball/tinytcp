//----------------------------------------------------------------------------
//  Copyright 2015  Robert Kimball
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

#ifndef NETWORKINTERFACE_H
#define NETWORKINTERFACE_H

#include <cinttypes>

#include "Address.h"

class AddressConfiguration
{
public:
   Address Address;
   uint8_t SubnetMask[ Address::ProtocolSize ];
   uint8_t Gateway[ Address::ProtocolSize ];
   uint8_t BroadcastMACAddress[ Address::HardwareSize ];
};

class NetworkInterface
{
public:
   static void RxData( void* data, size_t length );
   virtual void TxData( void* data, size_t length ) = 0;
};

void NetworkRxData( void* data, size_t length );

#endif
