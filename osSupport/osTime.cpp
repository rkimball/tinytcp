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
#elif __linux__
#include <time.h>
#endif
#include <stdio.h>

#include "osTime.hpp"

uint64_t osTime::GetTime()
{
#ifdef _WIN32
    FILETIME ftime;
    uint64_t time;

    // Time in 100ns ticks
    GetSystemTimeAsFileTime(&ftime);

    time = ftime.dwHighDateTime;
    time <<= 32;
    time += ftime.dwLowDateTime;

    return time;
#elif __linux__
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t rc = ts.tv_sec * 1000000 + (ts.tv_nsec / 1000);
    return rc;
#endif
}

const char* osTime::GetTimestamp()
{
    static char s[64];
    uint32_t    seconds;
    uint32_t    sec;
    uint32_t    min;
    uint32_t    hour;
    uint64_t    time;

    //printf( "The time is %.19s.%hu %s", timeline, timebuffer.millitm, &timeline[20] );
    time = GetTime();

    seconds = (uint32_t)(time / 1000000);

    sec = seconds % 60;

    min = seconds / 60;
    min %= 60;

    hour = seconds / (60 * 60);
    hour %= 24;

#ifdef OSTIME_SHOW_PROCESSOR_TIMESTAMP
    struct _timeb timebuffer;
    _ftime(&timebuffer);
    char* timeline;
    timeline = ctime(&(timebuffer.time));
    timeline = strchr(timeline, ':') - 2;
    snprintf(s,
             sizeof(s),
             "%.8s.%hu(%u:%02u:%02u.%03u)",
             timeline,
             timebuffer.millitm,
             hour,
             min,
             sec,
             (uint32_t)((time / 1000) % 1000));
#else
    snprintf(s, sizeof(s), "%u:%02u:%02u.%03u", hour, min, sec, (uint32_t)((time / 1000) % 1000));
#endif
    return s;
}
