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

#ifndef OSMUTEX_H
#define OSMUTEX_H

#ifdef _WIN32
#elif __linux__
#include <pthread.h>
#endif

#include "osPrintfInterface.h"

class osThread;

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
#ifdef _WIN32
   void* Handle;
#elif __linux__
   pthread_mutex_t m_mutex;

   // Can't use osMutex to lock the MutexList because you can't create an osMutex
   // without locking the MutexList, so make a private mutex
   static pthread_mutex_t MutexListMutex;
#endif

   static void StaticInit();
   static void LockListMutex();
   static void UnlockListMutex();

   bool Take();
   const char* Name;
   static osMutex* MutexList[];

   const char* OwnerFile;
   int OwnerLine;
   osThread* OwnerThread;
};

#endif
