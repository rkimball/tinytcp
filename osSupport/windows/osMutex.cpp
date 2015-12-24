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

#include <Windows.h>
#include <stdio.h>

#include "..\osMutex.h"

osMutex* osMutex::MutexList[ MAX_MUTEX ];

osMutex::osMutex( const char* name ) :
   Name( name )
{
   Handle = CreateMutex( NULL, false, name );
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
   ReleaseMutex( Handle );
}

bool osMutex::Take( const char* file, int line )
{
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
}

bool osMutex::Take()
{
   return WaitForSingleObject( Handle, INFINITE ) == 0;
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