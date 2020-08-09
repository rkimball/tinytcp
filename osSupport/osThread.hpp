//----------------------------------------------------------------------------
// Copyright(c) 2015-2020, Robert Kimball
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

#pragma once

#include <inttypes.h>
#include <iostream>

#ifndef _WIN32
#include <pthread.h>
#include <sched.h>
#include <setjmp.h>
#endif

#include "osEvent.hpp"

typedef void (*ThreadEntryPtr)(void*);

class osThread
{
public:
    typedef enum { INIT, RUNNING, PENDING_MUTEX, PENDING_EVENT, SLEEPING } THREAD_STATE;

    osThread();

    ~osThread();

    static void Initialize();

    int Create(ThreadEntryPtr entry, const char* name, int stack, int priority, void* param);

    void WaitForExit(int32_t millisecondWaitTimeout = -1);

    static void Sleep(unsigned long ms, const char* file, int line);

    static void USleep(unsigned long us, const char* file, int line);

    void SetState(THREAD_STATE state, const char* file, int line, void* obj);

    void ClearState();

    static osThread* GetCurrent();

    const char* GetName();

#ifdef _WIN32
    void* GetHandle();

    uint32_t GetThreadId();
    uint32_t ThreadId;

    void* Handle;
#elif __linux__
    uint32_t GetHandle();

    pthread_t      m_thread;
    ThreadEntryPtr Entry;
    void*          Param;
#endif
    osEvent ThreadStart;

    static void dump_info(std::ostream&);

    static const int32_t STATE_LENGTH_MAX = 81;
    int32_t              Priority;
    unsigned long        USleepTime;
    static const int32_t NAME_LENGTH_MAX = 32;

private:
    char         Name[NAME_LENGTH_MAX];
    const char*  Filename;
    int          Linenumber;
    THREAD_STATE State;
    void*        StateObject;
};
