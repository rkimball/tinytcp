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
