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

#include <iostream>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <pcap.h>
#elif __linux__
#include <strings.h>
#include <unistd.h>
#endif

#include "DefaultStack.hpp"
#include "InterfaceMAC.hpp"
#include "PacketIO.hpp"
#include "ProtocolARP.hpp"
#include "ProtocolDHCP.hpp"
#include "ProtocolTCP.hpp"
#include "http_page.hpp"
#include "httpd.hpp"
#include "osMutex.hpp"
#include "osThread.hpp"
#include "osTime.hpp"

#ifdef WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

PacketIO* PIO;
static osThread NetworkThread;
static osThread MainThread;

static osEvent StartEvent("StartEvent");

DefaultStack tcpStack;

struct NetworkConfig
{
    int interfaceNumber;
};

//============================================================================
// Callback function invoked by libpcap for every incoming packet
//============================================================================

#ifdef _WIN32
void packet_handler(u_char* param, const struct pcap_pkthdr* header, const u_char* pkt_data)
{
    ProtocolMACEthernet::ProcessRx((uint8_t*)pkt_data, header->len);
}
#elif __linux__
#endif

void RxData(uint8_t* data, size_t length)
{
    tcpStack.ProcessRx(data, length);
}

void TxData(void* data, size_t length)
{
    PIO->TxData(data, length);
}

void NetworkEntry(void* param)
{
    // This is just a made-up MAC address to user for testing
    uint8_t addr[] = {0x10, 0xBF, 0x48, 0x44, 0x55, 0x66};
    tcpStack.SetMACAddress(addr);

    //   Config.IPv4.Address[ 0 ] = 0;
    //   Config.IPv4.Address[ 1 ] = 0;
    //   Config.IPv4.Address[ 2 ] = 0;
    //   Config.IPv4.Address[ 3 ] = 0;

#ifdef _WIN32
    NetworkConfig& config = *(NetworkConfig*)param;
    char device[256];

    PacketIO::GetDevice(config.interfaceNumber, device, sizeof(device));
    printf("using device %s\n", device);
    // PacketIO::GetMACAddress( device, Config.MACAddress );

    PIO = new PacketIO(device);

    ProtocolMACEthernet::Initialize(PIO);

    StartEvent.Notify();

    // This method does not return...ever
    PIO->Start(packet_handler);
#elif __linux__
    PIO = new PacketIO();
    tcpStack.RegisterDataTransmitHandler(TxData);
    StartEvent.Notify();
    PIO->Start(RxData);
#endif
}

void MainEntry(void* config) {}

void HomePage(http::Page* page)
{
    time_t t = time(nullptr);
    struct tm* now = localtime(&t);
    std::ostream& out = page->get_output_stream();

    out << "<span>Current time: " << asctime(now) << "</span>\n";

    out << "<table class=\"table table-striped\">\n";
    out << "  <thead>\n";
    out << "    <th>Protocol</th>\n";
    out << "    <th>Size of class</th>\n";
    out << "  </thead>\n";
    out << "  <tbody>\n";
    out << "    <tr><td>MAC</td><td>" << sizeof(tcpStack.MAC) << "</td></tr>\n";
    out << "     <tr><td>IP</td><td>" << sizeof(tcpStack.IP) << "</td></tr>\n";
    out << "    <tr><td>TCP</td><td>" << sizeof(tcpStack.TCP) << "</td></tr>\n";
    out << "    <tr><td>ARP</td><td>" << sizeof(tcpStack.ARP) << "</td></tr>\n";
    out << "   <tr><td>ICMP</td><td>" << sizeof(tcpStack.ICMP) << "</td></tr>\n";
    out << "   <tr><td>DHCP</td><td>" << sizeof(tcpStack.DHCP) << "</td></tr>\n";
    out << "  </tbody>\n";
    out << "</table>\n";
}

void FormsDemo(http::Page* page)
{
    std::ostream& out = page->get_output_stream();
    out << "<form action=\"/formsresult\">";

    out << "<label for=\"FirstName\">First name:</label>";
    out << "<input type=\"text\" name=\"FirstName\" class=\"form-control\" value=\"Robert\"/>";
    out << "<br>";

    out << "<label for=\"LastName\">Last name:</label>";
    out << "<input type=\"text\" name=\"LastName\" class=\"form-control\" value=\"Kimball\"/>";
    out << "<br>";

    out << "<input type=\"submit\" value=\"submit\" />";

    //      page->Reference( "/files/test1.zip", "test1.zip" );
    //      page->SendString( "      <form action=\"/test/uploadfile\" method=\"POST\" " );
    //      page->SendString( "      enctype=\"multipart/form-data\" action=\"_URL_\">\n" );
    //      page->SendString( "File: <input type=\"file\" name=\"file\" size=\"50\"><br>\n" );
    //      page->SendString( "      <input type=\"submit\" value=\"Upload\">\n" );
    //      page->SendString( "      </form><br>\n" );
}

void ShowMutex(http::Page* page)
{
    std::ostream& out = page->get_output_stream();
    out << "<pre>";
    osMutex::dump_info(out);
    out << "</pre>";
}

void ShowEvent(http::Page* page)
{
    std::ostream& out = page->get_output_stream();
    out << "<pre>";
    osEvent::dump_info(out);
    out << "</pre>";
}

void ShowQueue(http::Page* page)
{
    std::ostream& out = page->get_output_stream();
    out << "<pre>";
    osQueue::dump_info(out);
    out << "</pre>";
}

void ShowThread(http::Page* page)
{
    std::ostream& out = page->get_output_stream();
    out << "<pre>";
    osThread::dump_info(out);
    out << "</pre>";
}

void ShowMAC(http::Page* page)
{
    std::ostream& out = page->get_output_stream();
    out << "<pre>";
    out << tcpStack.MAC;
    out << "</pre>";
}

void ShowIP(http::Page* page)
{
    std::ostream& out = page->get_output_stream();
    out << "<pre>";
    out << tcpStack.IP;
    out << "</pre>";
}

void ShowARP(http::Page* page)
{
    std::ostream& out = page->get_output_stream();
    out << "<pre>";
    out << tcpStack.ARP;
    out << "</pre>";
}

void ShowTCP(http::Page* page)
{
    std::ostream& out = page->get_output_stream();
    out << "<pre>";
    out << tcpStack.TCP;
    out << "</pre>";
}

void FormsResponse(http::Page* page)
{
    for (int i = 0; i < page->argc; i++)
    {
        char* name;
        char* value;
        page->ParseArg(page->argv[i], &name, &value);
        page->Printf("<span>%s = %s</span>", name, value);
        page->Printf("<br>");
    }
}

void Page404(http::Page* page)
{
    page->Printf("<div class=\"jumbotron>");
    page->Printf("<h1>Page Not Found</h1>");
    page->Printf("</div>");
}

void ProcessPageRequest(http::Page* page, const char* url)
{
    if (!strcasecmp(url, "/"))
    {
        page->Process(BINARY_DIR "master.html", "$content", HomePage);
    }
    else if (!strcasecmp(url, "/forms"))
    {
        page->Process(BINARY_DIR "master.html", "$content", FormsDemo);
    }
    else if (!strcasecmp(url, "/formsresult"))
    {
        page->Process(BINARY_DIR "master.html", "$content", FormsResponse);
    }
    //   else if( !strcasecmp( url, "/files/test1.zip" ) )
    //   {
    //      page->PageOK();
    //      page->SendFile( "c:\\test.rar" );
    //   }
    //   else if( !strcasecmp( url, "/test/uploadfile" ) )
    //   {
    //      printf( "Reading %d bytes\n", page->ContentLength );
    //      for( int i = 0; i<page->ContentLength; i++ )
    //      {
    //         page->Connection->Read();
    //      }
    //      printf( "Done reading\n" );
    //      page->PageOK();
    //      page->Printf( "Upload %d bytes complete\n", page->ContentLength );
    //   }
    else if (!strcasecmp(url, "/show/mac"))
    {
        page->Process(BINARY_DIR "master.html", "$content", ShowMAC);
    }
    else if (!strcasecmp(url, "/show/ip"))
    {
        page->Process(BINARY_DIR "master.html", "$content", ShowIP);
    }
    else if (!strcasecmp(url, "/show/arp"))
    {
        page->Process(BINARY_DIR "master.html", "$content", ShowARP);
    }
    else if (!strcasecmp(url, "/show/tcp"))
    {
        page->Process(BINARY_DIR "master.html", "$content", ShowTCP);
    }
    else if (!strcasecmp(url, "/show/thread"))
    {
        page->Process(BINARY_DIR "master.html", "$content", ShowThread);
    }
    else if (!strcasecmp(url, "/show/queue"))
    {
        page->Process(BINARY_DIR "master.html", "$content", ShowQueue);
    }
    else if (!strcasecmp(url, "/show/event"))
    {
        page->Process(BINARY_DIR "master.html", "$content", ShowEvent);
    }
    else if (!strcasecmp(url, "/show/mutex"))
    {
        page->Process(BINARY_DIR "master.html", "$content", ShowMutex);
    }
    else
    {
        page->PageNotFound();
    }
}

int main(int argc, char* argv[])
{
    NetworkConfig config;
    config.interfaceNumber = 1;
    http::Server WebServer;

    printf("%d bit build\n", (sizeof(void*) == 4 ? 32 : 64));

    // Start at 1 to skip the file name
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-devices"))
        {
            PacketIO::DisplayDevices();
            return 0;
        }
        else if (!strcmp(argv[i], "-use"))
        {
            config.interfaceNumber = atoi(argv[++i]);
        }
        else
        {
            printf("unknown option '%s'\n", argv[i]);
            return -1;
        }
    }

    WebServer.RegisterPageHandler(ProcessPageRequest);
    NetworkThread.Create(NetworkEntry, "Network", 1024, 10, &config);

#ifdef _WIN32
    Sleep(1000);
#elif __linux__
    usleep(1000000);
#endif
    StartEvent.Wait(__FILE__, __LINE__);

    WebServer.Initialize(tcpStack.MAC, tcpStack.TCP, 80);

    tcpStack.StartDHCP();

    while (1)
    {
#ifdef _WIN32
        Sleep(100);
#elif __linux__
        usleep(100000);
#endif
        tcpStack.Tick();
    }
}
