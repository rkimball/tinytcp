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

#include <stdio.h>

#include "Utility.hpp"

void DumpData(std::ostream& out, void* buffer, size_t len)
{
    std::ios_base::fmtflags flags = out.flags();
    const uint8_t* data = reinterpret_cast<const uint8_t*>(buffer);
    size_t index = 0;
    while (index < len)
    {
        out << std::hex << std::setw(8) << std::setfill('0') << index;
        for (int i = 0; i < 8; i++)
        {
            if (index + i < len)
            {
                out << " " << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)data[i];
            }
            else
            {
                out << "   ";
            }
        }
        out << "  ";
        for (int i = 8; i < 16; i++)
        {
            if (index + i < len)
            {
                out << " " << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)data[i];
            }
            else
            {
                out << "   ";
            }
        }
        out << "  ";
        for (int i = 0; i < 16; i++)
        {
            char ch = (index + i < len ? data[i] : ' ');
            out << ((ch < 32) ? '.' : ch);
        }
        out << "\n";
        data += 16;
        index += 16;
    }
    out.flags(flags);
}

void DumpBits(std::ostream& out, void* buf, size_t size)
{
    uint8_t* buffer = (uint8_t*)buf;

    for (size_t i = 0; i < size; i++)
    {
        out << std::setw(3) << i << " - ";
        for (uint8_t bitIndex = 0x80; bitIndex != 0; bitIndex >>= 1)
        {
            if (buffer[i] & bitIndex)
            {
                out << "1";
            }
            else
            {
                out << "0";
            }
        }
        out << "\n";
    }
}

const char* ipv4toa(uint32_t addr)
{
    static char rc[20];
    sprintf(rc,
            "%d.%d.%d.%d",
            (addr >> 24) & 0xFF,
            (addr >> 16) & 0xFF,
            (addr >> 8) & 0xFF,
            (addr >> 0) & 0xFF);
    return rc;
}

const char* ipv4toa(const uint8_t* addr)
{
    static char rc[20];
    sprintf(rc, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
    return rc;
}

const char* macaddrtoa(const uint8_t* addr)
{
    static char rc[20];
    sprintf(
        rc, "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    return rc;
}

uint8_t Unpack8(const uint8_t* p, size_t offset, size_t size)
{
    return p[offset];
}

uint16_t Unpack16(const uint8_t* p, size_t offset, size_t size)
{
    uint16_t rc = 0;
    for (int i = 0; i < size; i++)
    {
        rc <<= 8;
        rc |= p[offset++];
    }
    return rc;
}

uint32_t Unpack32(const uint8_t* p, size_t offset, size_t size)
{
    uint32_t rc = 0;
    for (int i = 0; i < size; i++)
    {
        rc <<= 8;
        rc |= p[offset++];
    }
    return rc;
}

size_t Pack8(uint8_t* p, size_t offset, uint8_t value)
{
    p[offset++] = value;
    return offset;
}

size_t Pack16(uint8_t* p, size_t offset, uint16_t value)
{
    p[offset++] = (value >> 8) & 0xFF;
    p[offset++] = value & 0xFF;
    return offset;
}

size_t Pack32(uint8_t* p, size_t offset, uint32_t value)
{
    p[offset++] = (value >> 24) & 0xFF;
    p[offset++] = (value >> 16) & 0xFF;
    p[offset++] = (value >> 8) & 0xFF;
    p[offset++] = value & 0xFF;
    return offset;
}

size_t PackBytes(uint8_t* p, size_t offset, const uint8_t* value, size_t count)
{
    for (int i = 0; i < count; i++)
    {
        p[offset++] = value[i];
    }
    return offset;
}

size_t PackFill(uint8_t* p, size_t offset, uint8_t value, size_t count)
{
    for (int i = 0; i < count; i++)
    {
        p[offset++] = value;
    }
    return offset;
}

int ReadLine(char* buffer, size_t size, int (*ReadFunction)())
{
    int i;
    char c;
    bool done = false;
    int bytesProcessed = 0;

    while (!done)
    {
        i = ReadFunction();
        if (i == -1)
        {
            *buffer = 0;
            break;
        }
        c = (char)i;
        bytesProcessed++;
        switch (c)
        {
        case '\r': *buffer++ = 0; break;
        case '\n':
            *buffer++ = 0;
            done = true;
            break;
        default: *buffer++ = c; break;
        }

        if (bytesProcessed == size)
        {
            break;
        }
    }

    *buffer = 0;
    return bytesProcessed;
}

bool AddressCompare(const uint8_t* a1, const uint8_t* a2, int length)
{
    for (int i = 0; i < length; i++)
    {
        if (a1[i] != a2[i])
        {
            return false;
        }
    }

    return true;
}
