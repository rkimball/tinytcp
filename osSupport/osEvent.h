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

#ifndef OSEVENT_H
#define OSEVENT_H

#include <cinttypes>

#include "osPrintfInterface.h"
#include "osMutex.h"

class osEvent
{
public:
   osEvent( const char* name );

   ~osEvent();

   void Notify();

   bool Wait( const char* file, int line, int msTimeout=-1 );

   const char* GetName();

   static void Show( osPrintfInterface* out );

private:
   void* Handle;
   static const int NAME_LENGTH_MAX = 80;
   char Name[ NAME_LENGTH_MAX ];

   static osMutex ListMutex;
   static const int INSTANCE_MAX = 20;
   static osEvent* InstanceList[];
};

#endif