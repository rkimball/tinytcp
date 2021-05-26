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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "HTTPD.hpp"
#include "HTTPPage.hpp"
#include "ProtocolTCP.hpp"
//#include "base64.h"
#include "osQueue.hpp"
#include "osThread.hpp"

#ifdef WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#elif __linux__
#include <strings.h>
#endif

#define MAX_ARGV 10

http::Server::Server()
    : PagePool("HTTPPage Pool", MAX_ACTIVE_CONNECTIONS, PagePoolBuffer)
    , Thread()
    , ListenerConnection(nullptr)
    , CurrentConnection(nullptr)
    , PageHandler(nullptr)
    , ErrorHandler(nullptr)
    , AuthHandler(nullptr)
{
}

void http::Server::RegisterPageHandler(PageRequestHandler handler)
{
    PageHandler = handler;
}

void http::Server::RegisterErrorHandler(ErrorMessageHandler handler)
{
    ErrorHandler = handler;
}

void http::Server::ProcessRequest(http::Page* page)
{
    int i;
    char buffer1[512];
    char buffer[256];
    int actualSizeRead;
    char* p;
    char* result;
    char* path;
    const char* url = "";
    char username[20];
    char password[20];
    int argc;
    char* argv[MAX_ARGV];
    TCPConnection* connection = page->Connection;

    page->HTTPHeaderSent = false;
    actualSizeRead = connection->ReadLine(buffer1, sizeof(buffer1));
    if (actualSizeRead == -1)
    {
        return;
    }

    strtok(buffer1, " ");
    path = strtok(nullptr, " ");

    username[0] = 0;
    password[0] = 0;
    do
    {
        // char* encryptionType;
        // char* loginString;
        actualSizeRead = connection->ReadLine(buffer, sizeof(buffer));
        if (buffer[0] == 0)
        {
            break;
        }

        // p = strtok(buffer, ":");
        // if (!strcasecmp("Authorization", p))
        // {
        //     encryptionType = strtok(0, " ");
        //     loginString = strtok(0, " ");
        //     if (!strcasecmp(encryptionType, "Basic"))
        //     {
        //         char s[80];
        //         char* tmp;

        //         DecodeBase64(loginString, s, sizeof(s));
        //         tmp = tokenizer.strtok(s, ":");
        //         if (tmp != nullptr)
        //         {
        //             strncpy(username, tmp, sizeof(username) - 1);
        //             tmp = tokenizer.strtok(0, ":");
        //             if (tmp != nullptr)
        //             {
        //                 strncpy(password, tmp, sizeof(password) - 1);
        //             }
        //         }
        //     }
        // }
        // else if (!strcasecmp("Content-Type", p))
        // {
        //     char* tmp = strtok(0, "");
        //     if (tmp != nullptr)
        //     {
        //         strncpy(CurrentPage->ContentType, tmp, sizeof(CurrentPage->ContentType));
        //     }
        // }
        // else if (!strcasecmp("Content-Length", p))
        // {
        //     char* tmp = strtok(0, " ");
        //     if (tmp != nullptr)
        //     {
        //         rc = sscanf(tmp, "%d", &page->ContentLength);
        //         if (rc != 1)
        //         {
        //             printf("could not get length\n");
        //         }
        //         else
        //         {
        //             printf("Content Length: %d\n", page->ContentLength);
        //         }
        //     }
        // }
    } while (buffer[0] != 0);

    if (path)
    {
        url = strtok(path, "?");
        if (url == nullptr)
        {
            url = "";
        }

        // Here is where we start parsing the arguments passed
        argc = 0;
        for (i = 0; i < MAX_ARGV; i++)
        {
            argv[i] = strtok(nullptr, "&");
            if (argv[i])
            {
                int argLength;
                int charValue;
                int j;

                argLength = strlen(argv[i]) + 1;

                // Translate any escaped characters in the argument
                p = argv[i];
                result = p;
                for (j = 0; j < argLength; j++)
                {
                    if (p[j] == '+')
                    {
                        *result++ = ' ';
                    }
                    else if (p[j] == '%')
                    {
                        char s[3];
                        // This is an escaped character
                        snprintf(s, sizeof(s), "%c%c", p[j + 1], p[j + 2]);
                        sscanf(s, "%x", &charValue);
                        j += 2;
                        *result++ = charValue;
                    }
                    else
                    {
                        *result++ = p[j];
                    }
                }
                argc++;
            }
            else
            {
                break;
            }
        }

        // Page.Initialize( connection );
        if (PageHandler)
        {
            page->argc = argc;
            page->argv = argv;
            PageHandler(page, url);
        }

        connection->Flush();
        connection->Close();
    }
}

void http::Server::Initialize(InterfaceMAC& mac, ProtocolTCP& tcp, uint16_t port)
{
    int i;

    for (i = 0; i < MAX_ACTIVE_CONNECTIONS; i++)
    {
        PagePoolPages[i]._Server = this;
        PagePool.Put(&PagePoolPages[i]);
    }

    ListenerConnection = tcp.NewServer(&mac, port);
    Thread.Create(http::Server::TaskEntry, "HTTPD", 1024 * 32, 100, this);
}

void http::Server::ConnectionHandlerEntry(void* param)
{
    Page* page = (Page*)param;

    page->_Server->ProcessRequest(page);

    page->_Server->PagePool.Put(page);
}

void http::Server::TaskEntry(void* param)
{
    Server* s = (Server*)param;
    s->Task();
}

void http::Server::Task()
{
    TCPConnection* connection;
    Page* page;

    while (1)
    {
        connection = ListenerConnection->Listen();

        // Spawn off a thread to handle this connection
        page = (Page*)PagePool.Get();
        if (page)
        {
            page->Initialize(connection);
            page->Thread.Create(ConnectionHandlerEntry, "Page", 1024, 100, page);
        }
        else
        {
            printf("Error: Out of pages\n");
        }
    }
}
