//----------------------------------------------------------------------------
// Copyright(c) 2015-2021, Robert Kimball
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

#ifdef __linux__
#include "pthread.h"
#endif

#include "osMutex.hpp"

class osEvent
{
public:
    osEvent(const char* name);

    ~osEvent();

    void Notify();

    bool Wait(const char* file, int line, int msTimeout = -1);

    const char* GetName();

    static void dump_info(std::ostream&);

private:
#ifdef _WIN32
    void* Handle;
#elif __linux__
    pthread_mutex_t m_mutex;
    pthread_cond_t m_condition;
    bool m_test;
#endif
    static const int NAME_LENGTH_MAX = 80;
    char Name[NAME_LENGTH_MAX];
    osThread* pending;

    static osMutex ListMutex;
    static const int INSTANCE_MAX = 20;
    static osEvent* InstanceList[];
};
