//----------------------------------------------------------------------------
// Copyright(c) 2015, Robert Kimball
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
#endif
#include <stdio.h>

#include "mutex.hpp"
#include "thread.hpp"

namespace os
{

Mutex* Mutex::MutexList[MAX_MUTEX];

#ifdef __linux__
// Can't use Mutex to lock the MutexList because you can't create an Mutex
// without locking the MutexList, so make a private mutex
pthread_mutex_t Mutex::MutexListMutex;
#endif

Mutex::Mutex(const char* name)
    : Name(name ? name : "")
    , OwnerFile(nullptr)
    , OwnerThread(nullptr)
{
#ifdef _WIN32
    Handle = CreateMutex(nullptr, false, name);
#elif __linux__
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_mutex, &attr);
#endif

    StaticInit();
    LockListMutex();
    for (int i = 0; i < MAX_MUTEX; i++)
    {
        if (MutexList[i] == nullptr)
        {
            MutexList[i] = this;
            break;
        }
    }
    UnlockListMutex();
}

Mutex::~Mutex()
{
    // Remove this mutex from the list so dump_info() does not use a dangling pointer
    LockListMutex();
    for (int i = 0; i < MAX_MUTEX; i++)
    {
        if (MutexList[i] == this)
        {
            MutexList[i] = nullptr;
        }
    }
    UnlockListMutex();
#ifdef _WIN32
    CloseHandle(Handle);
#elif __linux__
    pthread_mutex_destroy(&m_mutex);
#endif
}

void Mutex::Give()
{
    Thread* thread = Thread::GetCurrent();
    if (thread != OwnerThread) {}
    OwnerFile = nullptr;
    OwnerLine = 0;
    OwnerThread = nullptr;
#ifdef _WIN32
    ReleaseMutex(Handle);
#elif __linux__
    pthread_mutex_unlock(&m_mutex);
#endif
}

void Mutex::Take(const char* file, int line)
{
#ifdef _WIN32
    DWORD rc = -1;

    if (Handle)
    {
        Thread* thread = Thread::GetCurrent();
        if (thread)
        {
            thread->SetState(Thread::THREAD_STATE::PENDING_MUTEX, file, line, this);
        }
        rc = WaitForSingleObject(Handle, INFINITE);
        if (thread)
        {
            thread->ClearState();
            OwnerThread = thread;
        }
        OwnerFile = file;
        OwnerLine = line;
    }
#elif __linux__
    Thread* thread = Thread::GetCurrent();
    if (thread)
    {
        thread->SetState(Thread::PENDING_MUTEX, file, line, this);
    }
    pthread_mutex_lock(&m_mutex);
    if (thread)
    {
        thread->ClearState();
    }
    OwnerFile = file;
    OwnerLine = line;
#endif
}

void Mutex::Take()
{
#ifdef _WIN32
    WaitForSingleObject(Handle, INFINITE) == 0;
#elif __linux__
    pthread_mutex_lock(&m_mutex);
#endif
}

const char* Mutex::GetName()
{
    return Name;
}

void Mutex::StaticInit()
{
    static bool IsInitialized = false;
    if (!IsInitialized)
    {
        IsInitialized = true;
#ifdef __linux__
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&MutexListMutex, &attr);
#endif
    }
}

void Mutex::LockListMutex()
{
#ifdef __linux__
    pthread_mutex_lock(&MutexListMutex);
#endif
}

void Mutex::UnlockListMutex()
{
#ifdef __linux__
    pthread_mutex_unlock(&MutexListMutex);
#endif
}

void Mutex::dump_info(TextOutput output, void* context)
{
    Print(output,
          context,
          "--------------------+-------+--------------------+------+--------------------\n");
    Print(output, context, " Name               | State | Owner              | Line | File\n");
    Print(output,
          context,
          "--------------------+-------+--------------------+------+--------------------\n");

    LockListMutex();
    for (int i = 0; i < MAX_MUTEX; i++)
    {
        Mutex* mutex = MutexList[i];
        if (mutex != nullptr)
        {
            const char* state = "";
            const char* owner = "";
            int line = -1;
            const char* file = "";
            if (mutex->OwnerFile)
            {
                line = mutex->OwnerLine;
                file = mutex->OwnerFile;
            }
            Print(output,
                  context,
                  "%-20s|%-7s|%-20s|%-6d|%s\n",
                  mutex->Name,
                  state,
                  owner,
                  line,
                  file);
        }
    }
    UnlockListMutex();
}

} // namespace os
