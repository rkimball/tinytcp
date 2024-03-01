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

#ifdef _WIN32
#include <Windows.h>
#elif __linux__
#include <unistd.h>
#endif
#include <cassert>
#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <string>

#include "event.hpp"
#include "mutex.hpp"
#include "thread.hpp"
#include "util.hpp"

static const int MAX_THREADS = 48;
static osThread* Threads[MAX_THREADS];

static osMutex Mutex("Thread List");
#ifdef _WIN32
static DWORD dwTlsIndex;
#elif __linux__
static pthread_key_t tlsKey;
#endif
static osThread MainThread;
static bool IsInitialized = false;

typedef struct WinThreadParam
{
    ThreadEntryPtr Entry;
    void* Param;
    osThread* Thread;
} * WINTHREADPARAMPTR;

osThread::osThread()
    : ThreadStart("ThreadStart")
{
    USleepTime = 0;
    State = INIT;
    Filename = "";
    Linenumber = 0;
#ifdef _WIN32
    Handle = 0;
    ThreadId = 0;
#elif __linux__
#endif
}

osThread::~osThread() {}

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
            Threads[i] = nullptr;
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
            Threads[i] = nullptr;
            break;
        }
    }

    return nullptr;
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
        // ErrorExit( "TlsAlloc failed" );
    }
    TlsSetValue(dwTlsIndex, &MainThread);
#elif __linux__
    pthread_key_create(&tlsKey, nullptr);
#endif
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (Threads[i] == nullptr)
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
    int rc = 0;

    snprintf(Name, NAME_LENGTH_MAX, "%s", name);
    Priority = priority;

#ifdef _WIN32
    WINTHREADPARAMPTR threadParam;

    // Allocate heap memory for thread parameter
    threadParam =
        (WINTHREADPARAMPTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WinThreadParam));

    // Set thread parameter values
    threadParam->Entry = entry;
    threadParam->Param = param;
    threadParam->Thread = this;

    DWORD tid;

    // Create thread
    Handle = CreateThread(nullptr, 0, WinThreadEntry, threadParam, 0, &tid);
    ThreadId = tid;
#elif __linux__

    // Set thread parameter values
    Entry = entry;
    Param = param;
    pthread_create(&m_thread, nullptr, ThreadEntry, this);
    ThreadStart.Wait(__FILE__, __LINE__);
#endif

    Mutex.Take(__FILE__, __LINE__);
    for (i = 0; i < MAX_THREADS; i++)
    {
        if (Threads[i] == nullptr)
        {
            Threads[i] = this;
            break;
        }
    }
    Mutex.Give();

    return rc;
}

void osThread::WaitForExit(int32_t millisecondWaitTimeout)
{
#ifdef _WIN32
    if (millisecondWaitTimeout < 0)
    {
        millisecondWaitTimeout = INFINITE;
    }
    WaitForSingleObject(Handle, millisecondWaitTimeout);
#elif __linux__
#endif
}

void osThread::Sleep(unsigned long ms, const char* file, int line)
{
    osThread* thread = GetCurrent();

    if (thread != nullptr)
    {
        thread->Filename = file;
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

    if (thread != nullptr)
    {
        thread->Filename = file;
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
    State = state;
    StateObject = obj;
    Filename = file;
    Linenumber = line;
}

void osThread::ClearState()
{
    State = RUNNING;
    StateObject = nullptr;
    Filename = "";
    Linenumber = 0;
}

void osThread::dump_info(std::ostream& out)
{
#ifdef WIN32
    FILETIME creationTime;
    FILETIME exitTime;
    FILETIME kernelTime;
    FILETIME userTime;
    SYSTEMTIME sysTime;
    HANDLE handle;

    handle = GetCurrentProcess();
    out << "Process Priority   = 0x" << to_hex(GetPriorityClass(handle)) << "\n";
    if (GetProcessTimes(handle, &creationTime, &exitTime, &kernelTime, &userTime))
    {
        FileTimeToSystemTime(&kernelTime, &sysTime);
        out << "  Kernel Time      = ", out << std::setw(2) << std::setfill('0') << sysTime.wHour << ":";
        out << std::setw(2) << std::setfill('0') << sysTime.wMinute << ":";
        out << std::setw(2) << std::setfill('0') << sysTime.wSecond << ".";
        out << std::setw(2) << std::setfill('0') << sysTime.wMilliseconds << "\n";

        FileTimeToSystemTime(&userTime, &sysTime);
        out << "  User Time        = ", out << std::setw(2) << std::setfill('0') << sysTime.wHour << ":";
        out << std::setw(2) << std::setfill('0') << sysTime.wMinute << ":";
        out << std::setw(2) << std::setfill('0') << sysTime.wSecond << ".";
        out << std::setw(2) << std::setfill('0') << sysTime.wMilliseconds << "\n";
    }
#endif

    out << "\n\n\"Reading\" means that the thread is reading a device or network socket\n\n";

#ifdef WIN32
    out << "----+--------------------+-------+--------------+--------------+-----------------------"
           "---"
           "\n";
    out << "Pri |Thread Name         | ID    | Kernel Time  |  User Time   | State\n";
    out << "----+--------------------+-------+--------------+--------------+-----------------------"
           "---"
           "\n";
#elif __linux__
    out << "--------------------+------------------------------------\n";
    out << "Thread Name         | State\n";
    out << "--------------------+------------------------------------\n";
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

        out << std::setw(3) << priority << " |";
        out << std::setw(20) << std::left << thread->Name << "|";
        out << std::setw(6) << handle << " | ";

        FileTimeToSystemTime(&kernelTime, &sysTime);
        out << std::setw(2) << std::setfill('0') << sysTime.wHour << ":";
        out << std::setw(2) << std::setfill('0') << sysTime.wMinute << ":";
        out << std::setw(2) << std::setfill('0') << sysTime.wSecond << ".";
        out << std::setw(2) << std::setfill('0') << sysTime.wMilliseconds << " | ";

        FileTimeToSystemTime(&userTime, &sysTime);
        out << std::setw(2) << std::setfill('0') << sysTime.wHour << ":";
        out << std::setw(2) << std::setfill('0') << sysTime.wMinute << ":";
        out << std::setw(2) << std::setfill('0') << sysTime.wSecond << ".";
        out << std::setw(2) << std::setfill('0') << sysTime.wMilliseconds << " | ";

#elif __linux__
        out << std::setw(20) << std::left << thread->Name << "|";
#endif

        // Find out what this thread is doing
        switch (thread->State)
        {
        case INIT: out << "init\n"; break;
        case RUNNING: out << "running\n"; break;
        case PENDING_MUTEX:
        {
            osMutex* obj = (osMutex*)(thread->StateObject);
            if (obj != nullptr)
            {
                out << "pending on mutex \"" << obj->GetName() << "\"\n";
            }
            else
            {
                out << "pending on mutex \"UNKNOWN\"\n";
            }
            break;
        }
        case PENDING_EVENT:
        {
            osEvent* obj = (osEvent*)(thread->StateObject);
            if (obj != nullptr)
            {
                out << "pending event \"" << obj->GetName() << "\"\n";
            }
            else
            {
                out << "pending event \"UNKNOWN\"\n";
            }
            break;
        }
        case SLEEPING:
            out << "sleeping " << thread->USleepTime;
            out << " at " << thread->Filename << " " << thread->Linenumber;
            break;
        }
    }

    Mutex.Give();
}
