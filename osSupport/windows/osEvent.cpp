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
