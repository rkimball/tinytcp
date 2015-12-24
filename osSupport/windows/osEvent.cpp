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

#include "../osEvent.h"
#include "../osThread.h"

osEvent*    osEvent::InstanceList[ osEvent::INSTANCE_MAX ];
osMutex     osEvent::ListMutex( "Event List" );

osEvent::osEvent( const char* name )
{
   if( name )
   {
      strncpy( Name, name, NAME_LENGTH_MAX - 1 );
   }
   Handle = CreateEvent( NULL, true, false, name );
   ListMutex.Take( __FILE__, __LINE__ );
   for( int i = 0; i < INSTANCE_MAX; i++ )
   {
      if( InstanceList[ i ] == 0 )
      {
         InstanceList[ i ] = this;
         break;
      }
   }
   ListMutex.Give();
}

osEvent::~osEvent()
{
   ListMutex.Take( __FILE__, __LINE__ );
   for( int i = 0; i < INSTANCE_MAX; i++ )
   {
      if( InstanceList[ i ] == this )
      {
         InstanceList[ i ] = 0;
      }
   }
   ListMutex.Give();
   CloseHandle( Handle );
}

void osEvent::Notify()
{
   SetEvent( Handle );
}

bool osEvent::Wait( const char* file, int line, int msTimeout )
{
   uint32_t             rc = 0;
   osThread* caller = osThread::GetCurrent();
   caller->SetState( osThread::THREAD_STATE::PENDING_EVENT, file, line, this );
   if( msTimeout == -1 )
   {
      WaitForSingleObject( Handle, INFINITE );
   }
   else
   {
      rc = WaitForSingleObject( Handle, msTimeout );
   }
   caller->ClearState();
   return (rc) ? false : true;
}

const char* osEvent::GetName()
{
   return Name;
}

void osEvent::Show( osPrintfInterface* out )
{
   uint32_t   i;

   out->Printf( "Event                         |Thread Name         |State\n");
   out->Printf( "------------------------------+--------------------+------------\n" );
   for( i=0; i<INSTANCE_MAX; i++ )
   {
      osEvent* e = InstanceList[ i ];
      if( e )
      {
         out->Printf( "%-30s|%-20s|%-10s\n",
            e->GetName(), "", "" );
      }
   }
}
