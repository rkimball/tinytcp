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

#ifndef ADDRESS_H
#define ADDRESS_H

#include <cinttypes>

class Address
{
public:
   static const uint8_t HardwareSize = 6;
   static const uint8_t ProtocolSize = 4;

   uint8_t Hardware[ HardwareSize ];
   uint8_t Protocol[ ProtocolSize ];

   static bool Compare( uint8_t* a1, uint8_t* a2, int length );
};

#endif
