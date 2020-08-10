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

#include <inttypes.h>
#include <iomanip>
#include <iostream>
#include <stdio.h>

#include "osQueue.hpp"

using namespace std;

static const size_t MAX_QUEUE_COUNT = 20;
static osQueue* QueueList[MAX_QUEUE_COUNT];
osMutex QueueListLock("queue list lock");

osQueue::osQueue(const char* name, int count, void** dataBuffer)
    : Array(dataBuffer)
    , Name(name)
    , NextInIndex(0)
    , NextOutIndex(0)
    , MaxElements(count)
    , ElementCount(0)
    , Lock("osQueue")
{
    // Insert 'this' into the list of queues
    for (int i = 0; i < MAX_QUEUE_COUNT; i++)
    {
        if (QueueList[i] == nullptr)
        {
            QueueList[i] = this;
            break;
        }
    }
}

const char* osQueue::GetName()
{
    return Name;
}

void* osQueue::Peek()
{
    void* rc;

    Lock.Take(__FILE__, __LINE__);

    if (ElementCount != 0)
    {
        rc = Array[NextOutIndex];
    }
    else
    {
        rc = 0;
    }

    Lock.Give();
    return rc;
}

void* osQueue::Get()
{
    void* rc;

    Lock.Take(__FILE__, __LINE__);

    if (ElementCount != 0)
    {
        rc = Array[NextOutIndex];
        NextOutIndex = Increment(NextOutIndex);
        ElementCount--;
    }
    else
    {
        rc = 0;
    }

    Lock.Give();
    return rc;
}

bool osQueue::Put(void* item)
{
    int next;
    bool rc = true;

    Lock.Take(__FILE__, __LINE__);

    next = Increment(NextInIndex);
    if (ElementCount >= MaxElements)
    {
        // Queue is full, so don't add item
        rc = false;
    }
    else
    {
        Array[NextInIndex] = item;
        NextInIndex = next;
        ElementCount++;
    }

    Lock.Give();
    return rc;
}

int osQueue::GetCount()
{
    return ElementCount;
}

void osQueue::Flush()
{
    while (Get() != 0)
        ;
}

bool osQueue::Contains(void* object)
{
    bool rc = false;
    int index;
    int i;

    Lock.Take(__FILE__, __LINE__);

    index = NextOutIndex;

    for (i = 0; i < ElementCount; i++)
    {
        if (object == Array[index])
        {
            rc = true;
            break;
        }
        index = Increment(index);
    }

    Lock.Give();

    return rc;
}

void osQueue::dump_info(std::ostream& out)
{
    QueueListLock.Take(__FILE__, __LINE__);
    for (int i = 0; i < MAX_QUEUE_COUNT; i++)
    {
        osQueue* queue = QueueList[i];
        if (queue != nullptr)
        {
            out << "Queue " << queue->GetName();
            out << " is size " << queue->MaxElements;
            out << " and contains " << queue->GetCount() << " objects\n";
            // This is hanging
            // can't lock the queue and tx tcp frames as it can deadlock
            // tx tcp locks queues
            // might be a better way to do this
            //         queue->Lock.Take( __FILE__, __LINE__ );
            //         int      index = queue->NextOutIndex;
            //         for( int j = 0; j<queue->GetCount(); j++ )
            //         {
            //            pfunc->Printf( "   Object at 0x%08X\n", queue->Array[ index ] );
            //            index = queue->Increment( index );
            //         }
            //         queue->Lock.Give();
        }
    }

    QueueListLock.Give();
}
