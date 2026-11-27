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

#include "FCS.hpp"
#include <stdio.h>

namespace tinytcp
{

uint32_t FCS::ChecksumAdd(const uint8_t* buffer, int length, uint32_t checksum)
{
    int32_t i;

    // The data is a series of 16 bit words, the first octet of each being the high order half
    for (i = 0; i + 1 < length; i += 2)
    {
        checksum += (uint32_t)((buffer[i] << 8) | buffer[i + 1]);
    }

    // RFC 1071: an odd last octet is padded on the right with a zero octet
    if (i < length)
    {
        checksum += (uint32_t)(buffer[i] << 8);
    }

    return checksum;
}

uint16_t FCS::ChecksumComplete(uint32_t checksum)
{
    // RFC 1071: every carry out of the low 16 bits is added back in, and adding it back can
    // produce another carry, so keep going until there is none
    while ((checksum >> 16) != 0)
    {
        checksum = (checksum & 0xFFFF) + (checksum >> 16);
    }

    return (uint16_t)~checksum;
}

uint16_t FCS::Checksum(const uint8_t* buffer, int length)
{
    return ChecksumComplete(ChecksumAdd(buffer, length, 0));
}

} // namespace tinytcp
