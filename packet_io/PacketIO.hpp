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

#pragma once

#ifdef _WIN32
#include <pcap.h>
#endif
#include <inttypes.h>
#include "osThread.hpp"

class PacketIO
{
public:
    PacketIO();
    PacketIO(const char* name);

    typedef void (*RxDataHandler)(uint8_t* data, size_t length);
#ifdef _WIN32
    void Start(pcap_handler handler);
#elif __linux__
    void Start(RxDataHandler);
    void Entry(void* param);
#endif
    void Stop();
    void TxData(void* data, size_t length);
    static void GetDevice(int interfaceNumber, char* buffer, size_t buffer_size);
    static int GetMACAddress(const char* adapter, uint8_t* mac);
    static void DisplayDevices();
    static void GetInterface(char* name);

private:
#ifdef _WIN32
    const char* CaptureDevice;
    pcap_t*     adhandle;
#elif __linux__
    osThread EthernetRxThread;
    int      m_RawSocket;
    int      m_IfIndex;
#endif
};
