//----------------------------------------------------------------------------
// Copyright( c ) 2017, Robert Kimball
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

#include <iostream>

#include "gtest/gtest.h"
#include "PacketIO.hpp"

using namespace std;

//============================================================================
//
//============================================================================

void RxData(uint8_t* data, size_t length)
{
    cout << "Rx " << length << endl;
}

TEST(network, raw_socket)
{
#ifdef _WIN32
    NetworkConfig& config = *(NetworkConfig*)param;
    char           device[256];

    PacketIO::GetDevice(config.interfaceNumber, device, sizeof(device));
    printf("using device %s\n", device);
    //PacketIO::GetMACAddress( device, Config.MACAddress );

    PIO = new PacketIO(device);

    ProtocolMACEthernet::Initialize(PIO);

    StartEvent.Notify();

    // This method does not return...ever
    PIO->Start(packet_handler);
#elif __linux__
    PacketIO* PIO = new PacketIO();
    PIO->Start(RxData);
#endif
}