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

#include "Utility.h"

void DumpData( void* buffer, int len, PrintfFunctionPtr pfunc )
{
   int   i, j;
   unsigned char* buf = (unsigned char*)buffer;
   char  tmpBuf[90];
   int            tmpIndex = 0;
   int            size = sizeof(tmpBuf);
   int            count;

   if( buf == 0 )
   {
      return;
   }

   i = 0;
   j = 0;
   while( i+1 <= len )
   {
      count = _snprintf_s( &tmpBuf[tmpIndex], size, size, "%04X ", i );
      tmpIndex += count;
      size -= count;
      for( j=0; j<16; j++ )
      {
         if( i+j < len )
         {
            count = _snprintf_s( &tmpBuf[tmpIndex], size, size, "%02X ", buf[i+j] );
            tmpIndex += count;
            size -= count;
         }
         else
         {
            count = _snprintf_s( &tmpBuf[tmpIndex], size, size, "   " );
            tmpIndex += count;
            size -= count;
         }

         if( j == 7 )
         {
            count = _snprintf_s( &tmpBuf[tmpIndex], size, size, "- " );
            tmpIndex += count;
            size -= count;
         }
      }
      count = _snprintf_s( &tmpBuf[tmpIndex], size, size, "  " );
      tmpIndex += count;
      size -= count;
      for( j=0; j<16; j++ )
      {
         if( buf[i+j] >= 0x20 && buf[i+j] <= 0x7E )
         {
            count = _snprintf_s( &tmpBuf[tmpIndex], size, size, "%c", buf[i+j] );
            tmpIndex += count;
            size -= count;
         }
         else
         {
            count = _snprintf_s( &tmpBuf[tmpIndex], size, size, "." );
            tmpIndex += count;
            size -= count;
         }
         if( i+j+1 == len )
         {
            break;
         }
      }

      i += 16;

      (void)pfunc( "%s\n", tmpBuf );
      tmpIndex = 0;
      size = sizeof(tmpBuf);
   }
}

void DumpBits( void* buf, int size, PrintfFunctionPtr pfunc )
{
   int      bitIndex;
   int      i;
   unsigned char* buffer = (unsigned char*)buf;

   for( i=0; i<size; i++ )
   {
      (void)pfunc( "%3d - ", i );
      for( bitIndex = 0x80; bitIndex != 0; bitIndex >>= 1 )
      {
         if( buffer[i] & bitIndex )
         {
            (void)pfunc( "1" );
         }
         else
         {
            (void)pfunc( "0" );
         }
      }
      (void)pfunc( "\n" );
   }
}

