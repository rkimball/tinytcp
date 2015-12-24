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

#ifndef DataBuffer_H
#define DataBuffer_H

#include <cinttypes>

#define TX_BUFFER_COUNT (3)
#define RX_BUFFER_COUNT (4)

#define DATA_BUFFER_PAYLOAD_SIZE (512)

class DataBuffer
{
public:
   DataBuffer(){};
   
   uint8_t* Packet;
   int32_t  AcknowledgementNumber;
   int32_t  Time_us;
   uint16_t Length;
   uint16_t Remainder;
   bool  Disposable;

   void Initialize()
   {
      Packet = Data;
      Length = 0;
      Remainder = DATA_BUFFER_PAYLOAD_SIZE;
      Disposable = true;
   }

private:
   uint8_t    Data[ DATA_BUFFER_PAYLOAD_SIZE ];
   
   DataBuffer( DataBuffer& );
};

#endif

