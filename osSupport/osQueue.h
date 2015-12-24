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

#ifndef OSQUEUE_H
#define OSQUEUE_H

#include "osMutex.h"

class osQueue
{
public:

   osQueue();

   osQueue( int elementCount, const char* name );

   void Initialize( int count, const char* name );

   const char* GetName();

   void* Peek( void );

   void* Peek( int index );

   void* Get( void );

   bool Put( void* item );

   int GetCount( void );

   void Flush( void );

   bool Contains( void* object );

   static void Show( osPrintfInterface* pfunc );

private:
   int Increment( int index )
   {
      if( ++index >= MaxElements )
      {
         index = 0;
      }
      return index;
   }

   void**      Array;
   int         MaxElements;
   const char* Name;
   int         NextInIndex;
   int         NextOutIndex;
   int         ElementCount;
   osMutex     Lock;
};

#endif
