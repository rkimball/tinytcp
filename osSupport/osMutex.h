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

#ifndef OSMUTEX_H
#define OSMUTEX_H

#include "osThread.h"

#define MAX_MUTEX    (16)

class osMutex
{
   friend class osThread;
public:
   osMutex( const char* name );

   void Give();

   bool Take( const char* file, int line );

   const char* GetName();

   static void Show( osPrintfInterface* pfunc );

private:
   bool Take();
   const char* Name;
   void* Handle;
   static osMutex* MutexList[];

   const char* OwnerFile;
   int OwnerLine;
   osThread* OwnerThread;

   size_t   TakeCount;
};

#endif
