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

#include <stdio.h>
#include "FCS.h"

uint32_t FCS::ChecksumAdd( uint8_t* buffer, int length, uint32_t checksum )
{
   uint16_t value;
   int32_t i;

   // Don't need to add in the checksum field so only 9x16 bit words in header
   for( i=0; i<length/2; i++ )
   {
      value = (buffer[ i*2 ] << 8) | buffer[ i*2+1 ];
      checksum += (uint32_t)value;
   }

   return checksum;
}

uint16_t FCS::ChecksumComplete( uint32_t checksum )
{
   uint16_t sum;
   sum  = (checksum & 0xFFFF);
   sum += (checksum >> 16);
   sum = ~sum;

   return sum;
}

uint16_t FCS::Checksum( uint8_t* buffer, int length )
{
   return ChecksumComplete( ChecksumAdd( buffer, length, 0 ) );
}
