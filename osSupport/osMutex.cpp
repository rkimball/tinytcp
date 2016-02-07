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

#ifdef _WIN32
#include <Windows.h>
#endif
#include <stdio.h>

#include "osMutex.h"
#include "osThread.h"

osMutex* osMutex::MutexList[ MAX_MUTEX ];

osMutex::osMutex( const char* name ) :
   Name( name )
{
#ifdef _WIN32
   Handle = CreateMutex( NULL, false, name );
#elif __linux__
   pthread_mutex_init( &m_mutex, NULL );
#endif
}

void osMutex::Give()
{
   osThread* thread = osThread::GetCurrent();
   if( thread != OwnerThread )
   {
   }
   OwnerFile = "";
   OwnerLine = 0;
   OwnerThread = NULL;
#ifdef _WIN32
   ReleaseMutex( Handle );
#elif __linux__
   pthread_mutex_unlock( &m_mutex );
#endif
}

bool osMutex::Take( const char* file, int line )
{
#ifdef _WIN32
   DWORD    rc = -1;

   if( Handle )
   {
      osThread* thread = osThread::GetCurrent();
      if( thread )
      {
         thread->SetState( osThread::THREAD_STATE::PENDING_MUTEX, file, line, this );
      }
      rc = WaitForSingleObject( Handle, INFINITE );
      if( thread )
      {
         thread->ClearState();
         OwnerThread = thread;
      }
      OwnerFile = file;
      OwnerLine = line;

      TakeCount++;
   }

   return rc == 0;
#elif __linux__
   pthread_mutex_lock( &m_mutex );
   OwnerFile = file;
   OwnerLine = line;
#endif
}

bool osMutex::Take()
{
#ifdef _WIN32
   return WaitForSingleObject( Handle, INFINITE ) == 0;
#elif __linux__
   pthread_mutex_lock( &m_mutex );
#endif
}

const char* osMutex::GetName()
{
   return Name;
}

void osMutex::Show( osPrintfInterface* pfunc )
{
   pfunc->Printf( "--------------------+-------+--------------------+--------------+--------------------------\n" );
   pfunc->Printf( " Name               | State | Owner              |  User Time   | State\n" );
   pfunc->Printf( "--------------------+-------+--------------------+--------------+--------------------------\n" );

   for( int i = 0; i < MAX_MUTEX; i++ )
   {
      osMutex* mutex = MutexList[ i ];
      if( mutex != NULL )
      {
         pfunc->Printf( "%-20s", mutex->Name );
      }
   }
}
