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

#pragma once

#include <cstdio>
#include <inttypes.h>

#define TCP_MAX_CONNECTIONS (5)
#define TCP_RX_WINDOW_SIZE (256)

#define TX_BUFFER_COUNT (20)
#define RX_BUFFER_COUNT (20)

// The size of a buffer, which has to hold a whole Ethernet frame. RFC 791 says every host must
// accept an IP datagram of 576 octets, so the buffer holds the 14 octet Ethernet header and 576
// octets, rounded up to 592. Larger frames are dropped.
#define DATA_BUFFER_PAYLOAD_SIZE (592)

// The time to live of the IP datagrams the stack sends. RFC 1122 says a fixed value must be
// configurable, and the Assigned Numbers registry says 64.
#define IPV4_DEFAULT_TTL (64)

namespace tinytcp
{
const uint8_t ARPCacheSize = 5;
}
