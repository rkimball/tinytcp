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
#include <unistd.h>
#endif
#include <cassert>
#include <stdio.h>

#include "osEvent.hpp"
#include "osMutex.hpp"
#include "osThread.hpp"

#include <iostream>
using namespace std;
static const int MAX_THREADS = 48;
static osThread* Threads[MAX_THREADS];

static osMutex Mutex("Thread List");
#ifdef _WIN32
static DWORD dwTlsIndex;
#elif __linux__
static pthread_key_t tlsKey;
#endif
static osThread MainThread;
static bool     IsInitialized = false;

typedef struct WinThreadParam
{
    ThreadEntryPtr Entry;
    void*          Param;
    osThread*      Thread;
} * WINTHREADPARAMPTR;

osThread::osThread()
    : ThreadStart("ThreadStart")
{
    USleepTime = 0;
    State      = INIT;
    Filename   = "";
    Linenumber = 0;
#ifdef _WIN32
    Handle   = 0;
    ThreadId = 0;
#elif __linux__
#endif
}

osThread::~osThread()
{
}

#ifdef _WIN32
DWORD WINAPI WinThreadEntry(LPVOID param)
{
    WINTHREADPARAMPTR thread = (WINTHREADPARAMPTR)param;

    // Set the thread local storage to point to this osThread
    TlsSetValue(dwTlsIndex, thread->Thread);

    thread->Entry(thread->Param);

    // Now remove self from active thread list
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (Threads[i] == thread->Thread)
        {
            Threads[i] = NULL;
            break;
        }
    }

    return 0;
}
#elif __linux__
static void* ThreadEntry(void* param)
{
    osThread* thread = (osThread*)param;
    pthread_setspecific(tlsKey, thread); // Thread Local Storage points to osThread
    thread->ThreadStart.Notify();
    thread->Entry(thread->Param);

    // Now remove self from active thread list
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (Threads[i] == thread)
        {
            Threads[i] = NULL;
            break;
        }
    }

    return NULL;
}

#endif

const char* osThread::GetName()
{
    return Name;
}

#ifdef _WIN32
void* osThread::GetHandle()
{
    return Handle;
}

uint32_t osThread::GetThreadId()
{
    return ThreadId;
}
#endif

void osThread::Initialize()
{
    // This is a hack to add the thread that called main() to the list
    snprintf(MainThread.Name, NAME_LENGTH_MAX, "main");
#ifdef _WIN32
    if ((dwTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES)
    {
        //ErrorExit( "TlsAlloc failed" );
    }
    TlsSetValue(dwTlsIndex, &MainThread);
#elif __linux__
    pthread_key_create(&tlsKey, NULL);
    pthread_t mainThread = pthread_self();
#endif
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (Threads[i] == NULL)
        {
            Threads[i] = &MainThread;
            break;
        }
    }
    IsInitialized = true;
}

int osThread::Create(
    ThreadEntryPtr entry, const char* name, int stackSize, int priority, void* param)
{
    if (!IsInitialized)
    {
        Initialize();
    }
    int i;
    int j;
    int rc = 0;

    snprintf(Name, NAME_LENGTH_MAX, "%s", name);
    Priority = priority;

#ifdef _WIN32
    WINTHREADPARAMPTR threadParam;

    // Allocate heap memory for thread parameter
    threadParam =
        (WINTHREADPARAMPTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WinThreadParam));

    // Set thread parameter values
    threadParam->Entry  = entry;
    threadParam->Param  = param;
    threadParam->Thread = this;

    DWORD tid;

    // Create thread
    Handle   = CreateThread(NULL, 0, WinThreadEntry, threadParam, 0, &tid);
    ThreadId = tid;
#elif __linux__

    // Set thread parameter values
    Entry = entry;
    Param = param;
    pthread_create(&m_thread, NULL, ThreadEntry, this);
    ThreadStart.Wait(__FILE__, __LINE__);
#endif

    Mutex.Take(__FILE__, __LINE__);
    for (i = 0; i < MAX_THREADS; i++)
    {
        if (Threads[i] == NULL)
        {
            Threads[i] = this;
            break;
        }
    }
    Mutex.Give();

    return rc;
}

int osThread::WaitForExit(int32_t millisecondWaitTimeout)
{
#ifdef _WIN32
    if (millisecondWaitTimeout < 0)
    {
        millisecondWaitTimeout = INFINITE;
    }
    return WaitForSingleObject(Handle, millisecondWaitTimeout);
#elif __linux__
#endif
}

void osThread::Sleep(unsigned long ms, const char* file, int line)
{
    osThread* thread = GetCurrent();

    if (thread != 0)
    {
        thread->Filename   = file;
        thread->Linenumber = line;

        thread->USleepTime = ms * 1000;
#ifdef _WIN32
        SleepEx(ms, true);
#elif __linux__
        usleep(ms * 1000);
#endif
        thread->USleepTime = 0;
    }
    else
    {
#ifdef _WIN32
        SleepEx(ms, true);
#elif __linux__
        usleep(ms * 1000);
#endif
    }
}

void osThread::USleep(unsigned long us, const char* file, int line)
{
    osThread* thread = GetCurrent();

    if (thread != 0)
    {
        thread->Filename   = file;
        thread->Linenumber = line;

        thread->USleepTime = us;
#ifdef _WIN32
        SleepEx(us / 1000, true);
#elif __linux__
        usleep(us);
#endif
        thread->USleepTime = 0;
    }
    else
    {
#ifdef _WIN32
        SleepEx(us / 1000, true);
#elif __linux__
        usleep(us);
#endif
    }
}

osThread* osThread::GetCurrent()
{
#ifdef _WIN32
    return (osThread*)TlsGetValue(dwTlsIndex);
#elif __linux__
    return (osThread*)pthread_getspecific(tlsKey);
#endif
}

void osThread::SetState(THREAD_STATE state, const char* file, int line, void* obj)
{
    State       = state;
    StateObject = obj;
    Filename    = file;
    Linenumber  = line;
}

void osThread::ClearState()
{
    State       = RUNNING;
    StateObject = NULL;
    Filename    = "";
    Linenumber  = 0;
}

void osThread::Show(osPrintfInterface* pfunc)
{
#ifdef WIN32
    FILETIME   creationTime;
    FILETIME   exitTime;
    FILETIME   kernelTime;
    FILETIME   userTime;
    SYSTEMTIME sysTime;
    HANDLE     handle;

    handle = GetCurrentProcess();
    pfunc->Printf("Process Priority   = 0x%X\n", GetPriorityClass(handle));
    if (GetProcessTimes(handle, &creationTime, &exitTime, &kernelTime, &userTime))
    {
        FileTimeToSystemTime(&kernelTime, &sysTime);
        pfunc->Printf("  Kernel Time      = %02d:%02d:%02d.%03d\n",
                      sysTime.wHour,
                      sysTime.wMinute,
                      sysTime.wSecond,
                      sysTime.wMilliseconds);

        FileTimeToSystemTime(&userTime, &sysTime);
        pfunc->Printf("  User Time        = %02d:%02d:%02d.%03d\n",
                      sysTime.wHour,
                      sysTime.wMinute,
                      sysTime.wSecond,
                      sysTime.wMilliseconds);
    }
#endif

    pfunc->Printf(
        "\n\n\"Reading\" means that the thread is reading a device or network socket\n\n");

#ifdef WIN32
    pfunc->Printf(
        "----+--------------------+-------+--------------+--------------+--------------------------"
        "\n");
    pfunc->Printf("Pri |Thread Name         | ID    | Kernel Time  |  User Time   | State\n");
    pfunc->Printf(
        "----+--------------------+-------+--------------+--------------+--------------------------"
        "\n");
#elif __linux__
    pfunc->Printf("--------------------+------------------------------------\n");
    pfunc->Printf("Thread Name         | State\n");
    pfunc->Printf("--------------------+------------------------------------\n");
#endif

    Mutex.Take(__FILE__, __LINE__);

    for (int i = 0; i < MAX_THREADS; i++)
    {
        osThread* thread = Threads[i];
        if (!thread)
        {
            continue;
        }
#ifdef WIN32
        handle = thread->GetHandle();

        int priority;
        if (handle)
        {
            priority = GetThreadPriority(handle);
        }
        else
        {
            priority = -1;
        }
        GetThreadTimes(handle, &creationTime, &exitTime, &kernelTime, &userTime);

        pfunc->Printf("%3d |%-20s|%6u | ", priority, thread->Name, handle);

        FileTimeToSystemTime(&kernelTime, &sysTime);
        pfunc->Printf("%2d:%02d:%02d.%03d | ",
                      sysTime.wHour,
                      sysTime.wMinute,
                      sysTime.wSecond,
                      sysTime.wMilliseconds);

        FileTimeToSystemTime(&userTime, &sysTime);
        pfunc->Printf("%2d:%02d:%02d.%03d | ",
                      sysTime.wHour,
                      sysTime.wMinute,
                      sysTime.wSecond,
                      sysTime.wMilliseconds);
#elif __linux__
        pfunc->Printf("%-20s|", thread->Name);
#endif

        // Find out what this thread is doing
        switch (thread->State)
        {
        case INIT: pfunc->Printf("init\n"); break;
        case RUNNING: pfunc->Printf("running\n"); break;
        case PENDING_MUTEX:
        {
            osMutex* obj = (osMutex*)(thread->StateObject);
            if (obj != NULL)
            {
                pfunc->Printf("pending on mutex \"%s\"\n", obj->GetName());
            }
            else
            {
                pfunc->Printf("pending on mutex \"UNKNOWN\"\n");
            }
            break;
        }
        case PENDING_EVENT:
        {
            osEvent* obj = (osEvent*)(thread->StateObject);
            if (obj != NULL)
            {
                pfunc->Printf("pending event \"%s\"\n", obj->GetName());
            }
            else
            {
                pfunc->Printf("pending event \"UNKNOWN\"\n");
            }
            break;
        }
        case SLEEPING:
            pfunc->Printf("sleeping %luus at %s %d\n",
                          thread->USleepTime,
                          thread->Filename,
                          thread->Linenumber);
            break;
        }
    }

    Mutex.Give();
}
