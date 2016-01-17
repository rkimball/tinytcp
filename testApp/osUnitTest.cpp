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

#include <gtest/gtest.h>
#include "osUnitTest.h"
#include "osThread.h"
#include "osQueue.h"
#include "osEvent.h"
#include "osMutex.h"
#include "osTime.h"

static void ThreadEntry_t1( void* param )
{
   int* var = (int*)param;
   *var = 1;
}

static void ThreadEntry_t2( void* param )
{
   int* var = (int*)param;
   *var = 2;
}

void osUnitTest::osThreadTest()
{
   osThread t1;
   osThread t2;
   int var_t1 = 0;
   int var_t2 = 0;

   ASSERT_EQ( 0, t1.Create( ThreadEntry_t1, "t1", 1000, 1, &var_t1 ) );
   ASSERT_EQ( 0, t2.Create( ThreadEntry_t2, "t2", 1000, 1, &var_t2 ) );
   ASSERT_EQ( 0, t1.WaitForExit() );
   ASSERT_EQ( 0, t2.WaitForExit() );

   EXPECT_EQ( 1, var_t1 );
   EXPECT_EQ( 2, var_t2 );
}

void osUnitTest::osQueueTest()
{
   osQueue q1( 10, "test" );
   int data[] = { 0,1,2,3,4,5,6,7,8,9 };

   for( int i = 0; i < 10; i++ )
   {
      EXPECT_TRUE( q1.Put( &data[ i ] ) );
   }
   EXPECT_FALSE( q1.Put( nullptr ) );

   for( int i = 0; i < 10; i++ )
   {
      int j = *(int*)(q1.Get());
      EXPECT_EQ( i, j );
   }
   EXPECT_EQ( nullptr, q1.Get() );
}

void osUnitTest::osTimeTest()
{
}

void osUnitTest::osEventTest()
{
}

void osUnitTest::osMutexTest()
{
}
