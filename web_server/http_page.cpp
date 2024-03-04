//----------------------------------------------------------------------------
// Copyright(c) 2015-2021, Robert Kimball
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

#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "http_page.hpp"
#include "osThread.hpp"

#ifdef WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#elif __linux__
#include <strings.h>
#endif

template <typename T>
std::string to_hex(T obj, size_t width = sizeof(T) * 2)
{
    std::stringstream ss;
    ss << std::hex << std::setw(width) << std::setfill('0') << (size_t)obj;
    return ss.str();
}

http::Page::Page()
    : TagDepth(0)
    , StartTagOpen(false)
    , m_ostream(this)
    , m_istream(this)
    , m_put_back(5)
{
}

http::Page::~Page() {}

void http::Page::Initialize(TCPConnection* connection)
{
    Connection = connection;
    Busy = 0;
}

int http::Page::Printf(const char* format, ...)
{
    char buffer[BUFFER_SIZE];
    int i;
    va_list vlist;
    int rc;

    va_start(vlist, format);

    vsnprintf(buffer, sizeof(buffer), format, vlist);

    for (i = 0; buffer[i] && i < (int)(sizeof(buffer) - 1); i++)
    {
        RawSend(&buffer[i], 1);
    }
    rc = i;

    return rc;
}

std::string http::Page::HTMLEncode(const std::string& str)
{
    std::string rc;
    std::stringstream ss;
    for (size_t i = 0; i < str.size(); i++)
    {
        switch (str[i])
        {
        case '<': ss << "&lt"; break;

        case '>': ss << "&gt"; break;

        default: ss << str[i]; break;
        }
    }
    return ss.str();
}

bool http::Page::Puts(const char* string)
{
    int i;
    int length = (int)strlen(string);

    for (i = 0; i < length; i++)
    {
        switch (string[i])
        {
        case '\n': RawSend("<br>", 5); break;
        case '\r': break;
        default: RawSend(&string[i], 1); break;
        }
    }

    return true;
}

bool http::Page::SendString(const char* string)
{
    return RawSend(string, (int)strlen(string));
}

bool http::Page::RawSend(const void* p, size_t length)
{
    if (!HTTPHeaderSent)
    {
        PageOK();
    }
    Connection->Write((const uint8_t*)p, length);

    return true;
}

void http::Page::SendASCIIString(const char* string)
{
    char buffer[4];

    while (*string)
    {
        if (*string < 0x21 || *string > 0x7E)
        {
            snprintf(buffer, sizeof(buffer), "%%%02X", *string);
            RawSend(buffer, 3);
        }
        else
        {
            RawSend(string, 1);
        }
        string++;
    }
}

void http::Page::DumpData(const char* buffer, size_t length)
{
    size_t i;
    size_t j;
    std::ostream& out = get_output_stream();

    i = 0;
    j = 0;
    out << "<code>\n";
    while (i + 1 <= length)
    {
        out << to_hex((uint16_t)i) << " ";
        for (j = 0; j < 16; j++)
        {
            if (i + j < length)
            {
                out << to_hex(buffer[i + j]) << " ";
            }
            else
            {
                out << "   ";
            }

            if (j == 7)
            {
                out << "- ";
            }
        }
        out << "  ";
        for (j = 0; j < 16; j++)
        {
            if (buffer[i + j] >= 0x20 && buffer[i + j] <= 0x7E)
            {
                out << buffer[i + j];
            }
            else
            {
                out << ".";
            }
            if (i + j + 1 == length)
            {
                break;
            }
        }

        i += 16;

        out << "<br>\n";
    }
    out << "</code>\n";
}

void http::Page::PageNotFound()
{
    HTTPHeaderSent = true;
    std::ostream& out = get_output_stream();
    out << "HTTP/1.0 404 Not Found\r\n\r\n";
}

void http::Page::PageOK(const char* mimeType)
{
    HTTPHeaderSent = true;
    std::ostream& out = get_output_stream();
    out << "HTTP/1.0 200 OK\r\nContent-type: ";
    out << mimeType;
    out << "\r\n\r\n";
}

void http::Page::PageNoContent()
{
    HTTPHeaderSent = true;
    std::ostream& out = get_output_stream();
    out << "HTTP/1.0 204 No Content\r\nContent-type: text/html\r\n\r\n";
}

void http::Page::PageUnauthorized()
{
    HTTPHeaderSent = true;
    std::ostream& out = get_output_stream();
    out << "HTTP/1.0 401 Unauthorized\r\nContent-type: text/html\r\n\r\n";
}

bool http::Page::SendFile(const char* filename)
{
    char s[BUFFER_SIZE];
    FILE* f;
    bool rc = false;
    unsigned char buffer[512];
    uint64_t counti64;
    int count;
    std::ostream& out = get_output_stream();

    f = fopen(filename, "rb");
    if (f)
    {
#ifdef _WIN32
        _fseeki64(f, 0, SEEK_END);
        counti64 = _ftelli64(f);
        _fseeki64(f, 0, SEEK_SET);
#elif __linux__
        fseek(f, 0, SEEK_END);
        counti64 = ftell(f);
        fseek(f, 0, SEEK_SET);
#endif
        out << "HTTP/1.0 200 OK\r\nContent-Type: application/java-archive\r\n";
        out << "Content-Length: " << counti64 << "\r\n\r\n";
        SendString(s);

        do
        {
            count = (int)fread(buffer, 1, sizeof(buffer), f);
            if (count > 0)
            {
                RawSend(buffer, count);
            }
        } while (count > 0);

        rc = true;
    }

    return rc;
}

void http::Page::Flush()
{
    Connection->Flush();
}

void http::Page::ParseArg(char* arg, char** name, char** value)
{
    *name = arg;
    while (*arg)
    {
        if (*arg == '=')
        {
            *arg = 0;
            *value = ++arg;
            break;
        }
        arg++;
    }
}

void http::Page::Process(const char* htmlFile, const char* marker, MarkerContent content)
{
    FILE* f = fopen(htmlFile, "r");
    if (f)
    {
        int c;
        int markerIndex = 0;
        int markerLength = strlen(marker);
        std::ostream& out = get_output_stream();
        while ((c = fgetc(f)) > 0)
        {
            if (c == marker[markerIndex])
            {
                markerIndex++;
                if (markerIndex == markerLength)
                {
                    // found the marker
                    markerIndex = 0;
                    out << "<!-- ";
                    out << marker;
                    out << " start -->";
                    content(this);
                    out << "<!-- ";
                    out << marker;
                    out << " end -->";
                }
            }
            else if (markerIndex > 0)
            {
                // Send the part of the marker that matched so far
                out.write(marker, markerIndex);
                markerIndex = 0;
            }
            else
            {
                char ch = c;
                out << ch;
            }
        }
    }
    else
    {
        printf("failed to open file '%s'\n", htmlFile);
    }
}

std::ostream& http::Page::get_output_stream()
{
    return m_ostream;
}

std::istream& http::Page::get_input_stream()
{
    return m_istream;
}

std::streamsize http::Page::xsputn(const char* s, std::streamsize n)
{
    Connection->Write((const uint8_t*)s, n);
    return n;
}

int http::Page::overflow(int c)
{
    uint8_t ch = c;
    Connection->Write(&ch, 1);
    return c;
}

std::streambuf::int_type http::Page::underflow()
{
    std::streambuf::int_type rc;
    if (gptr() < egptr()) // buffer not exhausted
    {
        rc = traits_type::to_int_type(*gptr());
    }
    else
    {
        char* base = &m_char_buffer.front();
        char* start = base;

        if (eback() == base) // true when this isn't the first fill
        {
            // Make arrangements for putback characters
            std::memmove(base, egptr() - m_put_back, m_put_back);
            start += m_put_back;
        }

        // start is now the start of the buffer, proper.
        // Read from fptr_ in to the provided buffer
        int n = Connection->Read(start, m_char_buffer.size() - (start - base));
        if (n == 0)
        {
            rc = traits_type::eof();
        }
        else
        {
            // Set buffer pointers
            setg(base, start, start + n);
            rc = traits_type::to_int_type(*gptr());
        }
    }

    return rc;
}
