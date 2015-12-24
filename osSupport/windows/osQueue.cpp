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

#include "..\osQueue.h"

#include <iostream>
using namespace std;

static const size_t MAX_QUEUE_COUNT = 20;
static osQueue* QueueList[ MAX_QUEUE_COUNT ];
osMutex QueueListLock( "queue list lock" );

osQueue::osQueue() :
   Array( 0 ),
   Name( 0 ),
   ElementCount( 0 ),
   Lock( "osQueue" )
{
   for( int i = 0; i < MAX_QUEUE_COUNT; i++ )
   {
      if( QueueList[ i ] == NULL )
      {
         QueueList[ i ] = this;
         break;
      }
   }
}

osQueue::osQueue( int elementCount, const char* name ) :
   Array( 0 ),
   MaxElements( elementCount ),
   Name( name ),
   NextInIndex( 0 ),
   NextOutIndex( 0 ),
   ElementCount( 0 ),
   Lock( name )
{
   Array = new void*[ elementCount ];
   for( int i = 0; i < MAX_QUEUE_COUNT; i++ )
   {
      if( QueueList[ i ] == NULL )
      {
         QueueList[ i ] = this;
         break;
      }
   }
}

const char* osQueue::GetName()
{
   return Name;
}

void osQueue::Initialize( int elementCount, const char* name )
{
   MaxElements = elementCount;
   Name = name;
   NextInIndex = 0;
   NextOutIndex = 0;
   ElementCount = 0;

   if( Array )
   {
      delete[] Array;
   }

   Array = new void*[ elementCount ];
}

void* osQueue::Peek( void )
{
   void*    rc;

   Lock.Take( __FILE__, __LINE__ );

   if( ElementCount != 0 )
   {
      rc = Array[NextOutIndex];
   }
   else
   {
      rc = 0;
   }

   Lock.Give();
   return rc;
}

void* osQueue::Peek( int index )
{
   void*    rc;

   Lock.Take( __FILE__, __LINE__ );

   if( ElementCount != 0 && index < ElementCount )
   {
      rc = Array[(NextOutIndex+index)%MaxElements];
   }
   else
   {
      rc = 0;
   }

   Lock.Give();
   return rc;
}

void* osQueue::Get( void )
{
   void*    rc;

   Lock.Take( __FILE__, __LINE__ );

   if( ElementCount != 0 )
   {
      rc = Array[NextOutIndex];
      NextOutIndex = Increment( NextOutIndex );
      ElementCount--;
   }
   else
   {
      rc = 0;
   }

   Lock.Give();
   return rc;
}

bool osQueue::Put( void* item )
{
   int      next;
   bool     rc = true;

   Lock.Take( __FILE__, __LINE__ );

   next = Increment( NextInIndex );
   if( ElementCount >= MaxElements )
   {
      // Queue is full, so don't add item
      rc = false;
   }
   else
   {
      Array[NextInIndex] = item;
      NextInIndex = next;
      ElementCount++;
   }

   Lock.Give();
   return rc;
}

int osQueue::GetCount()
{
   return ElementCount;
}

void osQueue::Flush( void )
{
   while( Get() != 0 );
}

bool osQueue::Contains( void* object )
{
   bool     rc = false;
   int      index;
   int      i;

   Lock.Take( __FILE__, __LINE__ );

   index = NextOutIndex;

   for( i=0; i<ElementCount; i++ )
   {
      if( object == Array[index] )
      {
         rc = true;
         break;
      }
      index = Increment( index );
   }

   Lock.Give();

   return rc;
}

void osQueue::Show( osPrintfInterface* pfunc )
{
   int      index;
   int      i;

   QueueListLock.Take( __FILE__, __LINE__ );
   for( int i = 0; i < MAX_QUEUE_COUNT; i++ )
   {
      osQueue* queue = QueueList[ i ];
      if( queue != NULL )
      {
         queue->Lock.Take( __FILE__, __LINE__ );
         pfunc->Printf( "Queue %s contains %d objects\n", queue->GetName(), queue->GetCount() );
         for( i = 0; i<queue->GetCount(); i++ )
         {
            pfunc->Printf( "   Object at 0x%08X\n", queue->Array[ index ] );
         }
         queue->Lock.Give();
      }
   }

   QueueListLock.Give();
}

