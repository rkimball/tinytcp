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

#include "osStack.h"

#include <iostream>
#include <stdio.h>
#include <inttypes.h>
using namespace std;

static const size_t MAX_STACK_COUNT = 20;
static osStack* StackList[ MAX_STACK_COUNT ];
static osMutex StackListLock( "stack list lock" );

osStack::osStack( const char* name, int count, void** dataBuffer ) :
   Array( dataBuffer ),
   MaxElements( count ),
   Name( name ),
   Index( 0 ),
   ElementCount( 0 ),
   Lock( "osStack" )
{
   for( int i = 0; i < MAX_STACK_COUNT; i++ )
   {
      if( StackList[ i ] == NULL )
      {
         StackList[ i ] = this;
         break;
      }
   }
}

const char* osStack::GetName()
{
   return Name;
}

void* osStack::Peek()
{
   void* rc = NULL;
   if( Index > 0 )
   {
      rc = Array[ Index-1 ];
   }
   return rc;
}

void osStack::Push( void* value )
{
   if( Index < MaxElements )
   {
      Array[ Index++ ] = value;
   }
}

void* osStack::Pop()
{
   void* rc = NULL;
   if( Index > 0 )
   {
      rc = Array[ --Index ];
   }
   return rc;
}

void osStack::Show( osPrintfInterface* pfunc )
{
   int      i;

   StackListLock.Take( __FILE__, __LINE__ );
   for( int i = 0; i < MAX_STACK_COUNT; i++ )
   {
      osStack* stack = StackList[ i ];
      if( stack != NULL )
      {
         stack->Lock.Take( __FILE__, __LINE__ );
         pfunc->Printf( "Stack %s is size %d\n", stack->GetName(), stack->MaxElements );
         stack->Lock.Give();
      }
   }

   StackListLock.Give();
}

