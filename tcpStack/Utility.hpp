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

#pragma once

#include <inttypes.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>

void DumpData(std::ostream& out, void* buffer, size_t len);
void DumpBits(std::ostream& out, void* buffer, size_t size);

uint16_t ntoh16(uint16_t value);
uint16_t hton16(uint16_t value);
uint32_t ntoh32(uint32_t value);
uint32_t hton32(uint32_t value);

const char* ipv4toa(uint32_t addr);
const char* ipv4toa(const uint8_t* addr);
const char* macaddrtoa(const uint8_t* addr);

uint8_t Unpack8(const uint8_t* p, size_t offset, size_t size = 1);
uint16_t Unpack16(const uint8_t* p, size_t offset, size_t size = 2);
uint32_t Unpack32(const uint8_t* p, size_t offset, size_t size = 4);
size_t Pack8(uint8_t* p, size_t offset, uint8_t value);
size_t Pack16(uint8_t* p, size_t offset, uint16_t value);
size_t Pack32(uint8_t* p, size_t offset, uint32_t value);
size_t PackBytes(uint8_t* p, size_t offset, const uint8_t* value, size_t count);
size_t PackFill(uint8_t* p, size_t offset, uint8_t value, size_t count);
int ReadLine(char* buffer, size_t size, int (*ReadFunction)());

bool AddressCompare(const uint8_t* a1, const uint8_t* a2, int length);

template<typename T>
std::string to_dec(T obj, size_t width)
{
    std::stringstream ss;
    ss << std::setw(width) << std::setfill('0') << obj;
    return ss.str();
}

template<typename T>
std::string to_hex(T obj, size_t width=sizeof(T)*2)
{
    std::stringstream ss;
    ss << std::hex << std::setw(width) << std::setfill('0') << (size_t)obj;
    return ss.str();
}
