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

#include <iostream>
#include <vector>

#include "ProtocolTCP.hpp"
#include "osPrintfInterface.hpp"
#include "osThread.hpp"

namespace http
{
    class Page;
    class Server;
}

class http::Page : public osPrintfInterface, public std::streambuf
{
    friend class Server;

public:
    static const uint32_t BUFFER_SIZE = 512;
    typedef void (*MarkerContent)(http::Page*);

    void Initialize(TCPConnection*);

    int Printf(const char* format, ...);
    static void HTMLEncodef(osPrintfInterface*, const char* format, ...);

    /// Puts writes the string converting all newline characters to <br>
    bool Puts(const char* string);
    bool SendString(const char* string);
    bool RawSend(const void* buffer, size_t length);
    /// SendASCIIString converts all non-displayable characters
    /// to '%XX' where XX is the hex values of the character
    void SendASCIIString(const char* string);
    void DumpData(const char* buffer, size_t length);
    bool SendFile(const char* filename);

    void PageOK(const char* mimeType = "text/html");
    void PageNotFound();
    void PageNoContent();
    void PageUnauthorized();

    void ParseArg(char* arg, char** name, char** value);

    void Process(const char* htmlFile, const char* marker, MarkerContent);

    void Flush();

    std::ostream& get_output_stream();
    std::istream& get_input_stream();

    char* Directory;

    char   URL[256];
    char   ContentType[256];
    int    ContentLength;
    int    Busy;
    int    argc;
    char** argv;
    int    TagDepth;
    bool   StartTagOpen;

    TCPConnection* Connection;

private:
    Page(Page&);
    Page();
    ~Page();

    void ClosePendingOpen();

    std::streamsize xsputn(const char* s, std::streamsize n);
    std::streambuf::int_type overflow(std::streambuf::int_type c);
    std::streambuf::int_type underflow();

    osThread Thread;
    Server*  _Server;
    bool     HTTPHeaderSent;

    std::ostream      m_ostream;
    std::istream      m_istream;
    const std::size_t m_put_back;
    std::vector<char> m_char_buffer;
};
