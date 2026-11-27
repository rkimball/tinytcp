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
#elif __linux__
#include <unistd.h>
#endif
#include <cassert>
#include <stdio.h>
#include <string.h>

#include "event.hpp"
#include "mutex.hpp"
#include "thread.hpp"

namespace os
{

static const int MAX_THREADS = 48;
static Thread* Threads[MAX_THREADS];

static Mutex ThreadListMutex("Thread List");
#ifdef _WIN32
static DWORD dwTlsIndex;
#elif __linux__
static pthread_key_t tlsKey;
#endif
static Thread MainThread;
static bool IsInitialized = false;

typedef struct WinThreadParam
{
    ThreadEntryPtr Entry;
    void* Param;
    os::Thread* Thread;
}* WINTHREADPARAMPTR;

Thread::Thread()
    : ThreadStart("ThreadStart")
{
    // The name is set by Create(). Until then it is the empty string, never uninitialized.
    memset(Name, 0, sizeof(Name));
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

Thread::~Thread()
{
    // Remove this thread from the list so dump_info() does not use a dangling pointer
    ThreadListMutex.Take(__FILE__, __LINE__);
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (Threads[i] == this)
        {
            Threads[i] = nullptr;
        }
    }
    ThreadListMutex.Give();
}

#ifdef _WIN32
DWORD WINAPI WinThreadEntry(LPVOID param)
{
    WINTHREADPARAMPTR thread = (WINTHREADPARAMPTR)param;

    // Set the thread local storage to point to this Thread
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
    Thread* thread = (Thread*)param;
    pthread_setspecific(tlsKey, thread); // Thread Local Storage points to Thread
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

const char* Thread::GetName()
{
    return Name;
}

#ifdef _WIN32
void* Thread::GetHandle()
{
    return Handle;
}

uint32_t Thread::GetThreadId()
{
    return ThreadId;
}
#endif

void Thread::Initialize()
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

int Thread::Create(ThreadEntryPtr entry, const char* name, int stackSize, int priority, void* param)
{
    if (!IsInitialized)
    {
        Initialize();
    }
    int i;
    int rc = 0;

    // A null name becomes the empty string and a name that is too long is cut off.
    // snprintf always terminates Name.
    snprintf(Name, NAME_LENGTH_MAX, "%s", name ? name : "");
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

    ThreadListMutex.Take(__FILE__, __LINE__);
    for (i = 0; i < MAX_THREADS; i++)
    {
        if (Threads[i] == nullptr)
        {
            Threads[i] = this;
            break;
        }
    }
    ThreadListMutex.Give();

    return rc;
}

void Thread::WaitForExit(int32_t millisecondWaitTimeout)
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

void Thread::Sleep(unsigned long ms, const char* file, int line)
{
    Thread* thread = GetCurrent();

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

void Thread::USleep(unsigned long us, const char* file, int line)
{
    Thread* thread = GetCurrent();

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

Thread* Thread::GetCurrent()
{
    // The thread local storage key is created by Initialize(). Until then it is zero, which may be
    // a key that some other library owns, and reading it would return that library's data.
    if (!IsInitialized)
    {
        return nullptr;
    }
#ifdef _WIN32
    return (Thread*)TlsGetValue(dwTlsIndex);
#elif __linux__
    return (Thread*)pthread_getspecific(tlsKey);
#endif
}

void Thread::SetState(THREAD_STATE state, const char* file, int line, void* obj)
{
    State = state;
    StateObject = obj;
    Filename = file;
    Linenumber = line;
}

void Thread::ClearState()
{
    State = RUNNING;
    StateObject = nullptr;
    Filename = "";
    Linenumber = 0;
}

void Thread::dump_info(TextOutput output, void* context)
{
#ifdef WIN32
    FILETIME creationTime;
    FILETIME exitTime;
    FILETIME kernelTime;
    FILETIME userTime;
    SYSTEMTIME sysTime;
    HANDLE handle;

    handle = GetCurrentProcess();
    Print(output,
          context,
          "Process Priority   = 0x%lx\n",
          static_cast<unsigned long>(GetPriorityClass(handle)));
    if (GetProcessTimes(handle, &creationTime, &exitTime, &kernelTime, &userTime))
    {
        FileTimeToSystemTime(&kernelTime, &sysTime);
        Print(output,
              context,
              "  Kernel Time      = %02d:%02d:%02d.%02d\n",
              sysTime.wHour,
              sysTime.wMinute,
              sysTime.wSecond,
              sysTime.wMilliseconds);

        FileTimeToSystemTime(&userTime, &sysTime);
        Print(output,
              context,
              "  User Time        = %02d:%02d:%02d.%02d\n",
              sysTime.wHour,
              sysTime.wMinute,
              sysTime.wSecond,
              sysTime.wMilliseconds);
    }
#endif

    Print(output,
          context,
          "\n\n\"Reading\" means that the thread is reading a device or network socket\n\n");

#ifdef WIN32
    Print(
        output,
        context,
        "----+--------------------+-------+--------------+--------------+--------------------------"
        "\n");
    Print(output,
          context,
          "Pri |Thread Name         | ID    | Kernel Time  |  User Time   | State\n");
    Print(
        output,
        context,
        "----+--------------------+-------+--------------+--------------+--------------------------"
        "\n");
#elif __linux__
    Print(output, context, "--------------------+------------------------------------\n");
    Print(output, context, "Thread Name         | State\n");
    Print(output, context, "--------------------+------------------------------------\n");
#endif

    ThreadListMutex.Take(__FILE__, __LINE__);

    for (int i = 0; i < MAX_THREADS; i++)
    {
        Thread* thread = Threads[i];
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

        Print(output, context, "%3d |%-20s|%6p | ", priority, thread->Name, handle);

        FileTimeToSystemTime(&kernelTime, &sysTime);
        Print(output,
              context,
              "%02d:%02d:%02d.%02d | ",
              sysTime.wHour,
              sysTime.wMinute,
              sysTime.wSecond,
              sysTime.wMilliseconds);

        FileTimeToSystemTime(&userTime, &sysTime);
        Print(output,
              context,
              "%02d:%02d:%02d.%02d | ",
              sysTime.wHour,
              sysTime.wMinute,
              sysTime.wSecond,
              sysTime.wMilliseconds);

#elif __linux__
        Print(output, context, "%-20s|", thread->Name);
#endif

        // Find out what this thread is doing
        switch (thread->State)
        {
        case INIT: Print(output, context, "init\n"); break;
        case RUNNING: Print(output, context, "running\n"); break;
        case PENDING_MUTEX:
        {
            Mutex* obj = (Mutex*)(thread->StateObject);
            Print(output,
                  context,
                  "pending on mutex \"%s\"\n",
                  obj != nullptr ? obj->GetName() : "UNKNOWN");
            break;
        }
        case PENDING_EVENT:
        {
            Event* obj = (Event*)(thread->StateObject);
            Print(output,
                  context,
                  "pending event \"%s\"\n",
                  obj != nullptr ? obj->GetName() : "UNKNOWN");
            break;
        }
        case SLEEPING:
            Print(output,
                  context,
                  "sleeping %lu at %s %d\n",
                  thread->USleepTime,
                  thread->Filename,
                  thread->Linenumber);
            break;
        }
    }

    ThreadListMutex.Give();
}

} // namespace os
