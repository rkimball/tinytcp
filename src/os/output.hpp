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

#include <stddef.h>

#if defined(__GNUC__) || defined(__clang__)
#define OS_PRINTF_FORMAT(format_index, first_arg)                                                  \
    __attribute__((format(printf, format_index, first_arg)))
#else
#define OS_PRINTF_FORMAT(format_index, first_arg)
#endif

namespace os
{

// Where the text produced by the dump_info() diagnostics goes. The callback receives a piece of
// text, which is not NUL terminated and is not necessarily a whole line. context is the pointer
// the caller passed along with the callback, so a sink such as a stream, socket or UART can be
// reached without globals.
typedef void (*TextOutput)(void* context, const char* text, size_t length);

// printf-style formatting to a TextOutput. A single call produces at most 255 characters, any
// more is truncated. Nothing is written if output is null.
void Print(TextOutput output, void* context, const char* format, ...) OS_PRINTF_FORMAT(3, 4);

} // namespace os
