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
#endif
#include <stdio.h>

#include "osMutex.hpp"
#include "osThread.hpp"

osMutex* osMutex::MutexList[MAX_MUTEX];

#ifdef __linux__
// Can't use osMutex to lock the MutexList because you can't create an osMutex
// without locking the MutexList, so make a private mutex
pthread_mutex_t osMutex::MutexListMutex;
#endif

osMutex::osMutex(const char* name)
    : Name(name)
    , OwnerFile(NULL)
    , OwnerThread(NULL)
{
#ifdef _WIN32
    Handle = CreateMutex(NULL, false, name);
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
        if (MutexList[i] == NULL)
        {
            MutexList[i] = this;
            break;
        }
    }
    UnlockListMutex();
}

void osMutex::Give()
{
    osThread* thread = osThread::GetCurrent();
    if (thread != OwnerThread)
    {
    }
    OwnerFile   = NULL;
    OwnerLine   = 0;
    OwnerThread = NULL;
#ifdef _WIN32
    ReleaseMutex(Handle);
#elif __linux__
    pthread_mutex_unlock(&m_mutex);
#endif
}

void osMutex::Take(const char* file, int line)
{
#ifdef _WIN32
    DWORD rc = -1;

    if (Handle)
    {
        osThread* thread = osThread::GetCurrent();
        if (thread)
        {
            thread->SetState(osThread::THREAD_STATE::PENDING_MUTEX, file, line, this);
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
    osThread* thread = osThread::GetCurrent();
    if (thread)
    {
        thread->SetState(osThread::PENDING_MUTEX, file, line, this);
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

void osMutex::Take()
{
#ifdef _WIN32
    WaitForSingleObject(Handle, INFINITE) == 0;
#elif __linux__
    pthread_mutex_lock(&m_mutex);
#endif
}

const char* osMutex::GetName()
{
    return Name;
}

void osMutex::StaticInit()
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

void osMutex::LockListMutex()
{
#ifdef __linux__
    pthread_mutex_lock(&MutexListMutex);
#endif
}

void osMutex::UnlockListMutex()
{
#ifdef __linux__
    pthread_mutex_unlock(&MutexListMutex);
#endif
}

void osMutex::Show(osPrintfInterface* pfunc)
{
    pfunc->Printf(
        "--------------------+-------+--------------------+------+--------------------\n");
    pfunc->Printf(" Name               | State | Owner              | Line | File\n");
    pfunc->Printf(
        "--------------------+-------+--------------------+------+--------------------\n");

    LockListMutex();
    for (int i = 0; i < MAX_MUTEX; i++)
    {
        osMutex* mutex = MutexList[i];
        if (mutex != NULL)
        {
            if (mutex->OwnerFile)
            {
                pfunc->Printf("%-20s|%-7s|%-20s|%-6d|%s\n",
                              mutex->Name,
                              "",
                              "",
                              mutex->OwnerLine,
                              mutex->OwnerFile);
            }
            else
            {
                pfunc->Printf("%-20s|%-7s|%-20s|%-6s|%s\n", mutex->Name, "", "", "", "");
            }
        }
    }
    UnlockListMutex();
}
