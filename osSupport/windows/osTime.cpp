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

#include "../osTime.h"
//void osTime::SetTime( uint32_t seconds, uint32_t microseconds )
//{
//   // We don't want to change Windows' system time so we
//   // store the offset in us used when getting processor time
//   ProcessorTimeOffset = 0;
//   ProcessorTimeOffset = GetTime() - ( seconds * 1000000 ) + microseconds;
//}
//
uint64_t osTime::GetProcessorTime( void )
{
   FILETIME    ftime;
   uint64_t         time;

   GetSystemTimeAsFileTime( &ftime );

   time = ftime.dwHighDateTime;
   time <<= 32;
   time += ftime.dwLowDateTime;

   return time;
}

uint64_t osTime::GetTime( void )
{
   return GetProcessorTime();
}

const char* osTime::GetTimestamp()
{
   static char    s[64];
   uint32_t            seconds;
   uint32_t            sec;
   uint32_t            min;
   uint32_t            hour;
   uint64_t            time;

   //printf( "The time is %.19s.%hu %s", timeline, timebuffer.millitm, &timeline[20] );
   time = GetProcessorTime();

   seconds = (uint32_t)(time / 1000000);

   sec = seconds % 60;

   min = seconds / 60;
   min %= 60;

   hour = seconds / (60*60);
   hour %= 24;

#ifdef OSTIME_SHOW_PROCESSOR_TIMESTAMP
   struct _timeb timebuffer;
   _ftime( &timebuffer );
   char *timeline;
   timeline = ctime( & ( timebuffer.time ) );
   timeline = strchr( timeline, ':' ) - 2;
   snprintf( s, sizeof(s), "%.8s.%hu(%u:%02u:%02u.%03u)", timeline, timebuffer.millitm, hour, min, sec, (uint32_t)((time/1000)%1000 ) );
#else
   snprintf( s, sizeof(s), "%u:%02u:%02u.%03u", hour, min, sec, (uint32_t)((time/1000)%1000 ) );
#endif
   return s;
}
