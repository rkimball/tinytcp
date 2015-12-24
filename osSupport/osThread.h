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

#ifndef OSTHREAD_H
#define OSTHREAD_H

#include <cinttypes>

#ifndef _WIN32
#include <pthread.h>
#include <sched.h>
#include <setjmp.h>
#endif

#include "osPrintfInterface.h"

typedef void (*ThreadEntryPtr)(void *);

class osThread
{
public:
   typedef enum
   {
      INIT,
      RUNNING,
      PENDING_MUTEX,
      PENDING_EVENT,
      SLEEPING
   } THREAD_STATE;

   osThread();

   ~osThread();

   static void Initialize();

   int Create
   (
      ThreadEntryPtr entry,
      char*          name,
      int            stack,
      int            priority,
      void*          param
   );

   int WaitForExit( int32_t millisecondWaitTimeout = -1 );

   void Kill();

   static void Sleep( unsigned long ms, const char* file, int line );
   
   static void USleep( unsigned long us, const char* file, int line );
   
   void SetState( THREAD_STATE state, const char* file, int line, void* obj );
   
   void ClearState();
   
   static osThread* GetCurrent();

   static int GetCurrentThreadId();

   static osThread* GetThreadById( int threadId );

   const char* GetName();

#ifdef _WIN32
   void* GetHandle();

   uint32_t GetThreadId();
   uint32_t ThreadId;
#else
   uint32_t GetHandle();
#endif

   static void Show( osPrintfInterface* pfunc );
   
   void* Handle;

   static const int32_t    STATE_LENGTH_MAX = 81;
   int32_t                 Priority;
   unsigned long           USleepTime;
   static const int32_t    NAME_LENGTH_MAX = 32;
private:

   char                 Name[ NAME_LENGTH_MAX ];
   const char*          Filename;
   int                  Linenumber;
   THREAD_STATE         State; 
   void*                StateObject;
};

#endif


