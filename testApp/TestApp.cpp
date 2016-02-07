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

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <pcap.h>
#elif __linux__
#include <strings.h>
#include <unistd.h>
#endif

#include "PacketIO.h"
#include "ProtocolMACEthernet.h"
#include "Address.h"
#include "ProtocolTCP.h"
#include "ProtocolDHCP.h"
#include "ProtocolARP.h"
#include "osThread.h"
#include "osMutex.h"
#include "osTime.h"
#include "HTTPD.h"
#include "HTTPPage.h"
#include "NetworkInterface.h"

#ifdef WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

PacketIO* PIO;
static osThread NetworkThread;
static osThread MainThread;
static osMutex* Semaphore;
HTTPD WebServer;

static osEvent StartEvent( "StartEvent" );

struct NetworkConfig
{
   int interfaceNumber;
};

//============================================================================
// Callback function invoked by libpcap for every incoming packet
//============================================================================

#ifdef _WIN32
void packet_handler( u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data )
{
   ProtocolMACEthernet::ProcessRx( (uint8_t*)pkt_data, header->len );
}
#elif __linux__
#endif

//============================================================================
//
//============================================================================

void NetworkEntry( void* param )
{
   NetworkConfig& config = *(NetworkConfig*)param;
   char device[ 256 ];

   // This is just a made-up MAC address to user for testing
   Config.MACAddress[ 0 ] = 0x10;
   Config.MACAddress[ 1 ] = 0xBF;
   Config.MACAddress[ 2 ] = 0x48;
   Config.MACAddress[ 3 ] = 0x44;
   Config.MACAddress[ 4 ] = 0x55;
   Config.MACAddress[ 5 ] = 0x66;

//   Config.MACAddress[ 0 ] = 0x00;
//   Config.MACAddress[ 1 ] = 0x15;
//   Config.MACAddress[ 2 ] = 0x5d;
//   Config.MACAddress[ 3 ] = 0x1c;
//   Config.MACAddress[ 4 ] = 0xa5;
//   Config.MACAddress[ 5 ] = 0xee;

   Config.IPv4.Address[ 0 ] = 0;
   Config.IPv4.Address[ 1 ] = 0;
   Config.IPv4.Address[ 2 ] = 0;
   Config.IPv4.Address[ 3 ] = 0;

#ifdef _WIN32
   PacketIO::GetDevice( config.interfaceNumber, device, sizeof( device ) );
   printf( "using device %s\n", device );
   //PacketIO::GetMACAddress( device, Config.MACAddress );

   PIO = new PacketIO( device );

   ProtocolMACEthernet::Initialize( PIO );

   StartEvent.Notify();

   // This method does not return...ever
   PIO->Start( packet_handler );
#elif __linux__
   PIO = new PacketIO();
   ProtocolMACEthernet::Initialize( PIO );
   StartEvent.Notify();
   PIO->Start();
#endif
}

//============================================================================
//
//============================================================================

void MainEntry( void* config )
{
   NetworkThread.Create( NetworkEntry, "Network", 1024, 10, config );

#ifdef _WIN32
   Sleep( 1000 );
#elif __linux__
   usleep( 1000000 );
#endif
   StartEvent.Wait( __FILE__, __LINE__ );

   WebServer.Initialize( 80 );
}

//============================================================================
//
//============================================================================

void ProcessPageRequest
(
   HTTPPage* page,
   const char* url,
   int argc,
   char** argv
)
{
   if( !strcasecmp( url, "/" ) )
   {
      page->PageStart();
      page->SendString( "<html>\n" );
      page->SendString( "   <head>\n" );
      page->SendString( "      <title>Protocol Stack Test Page</title>\n" );
      page->SendString( "   </head>\n" );
      page->SendString( "   <body>\n" );

      time_t t = time(0);
      struct tm* now = localtime( &t );
      page->Printf( "Current time: %s\n", asctime( now ) );

//      page->Reference( "/files/test1.zip", "test1.zip" );
//      page->SendString( "      <form action=\"/test/uploadfile\" method=\"POST\" " );
//      page->SendString( "      enctype=\"multipart/form-data\" action=\"_URL_\">\n" );
//      page->SendString( "File: <input type=\"file\" name=\"file\" size=\"50\"><br/>\n" );
//      page->SendString( "      <input type=\"submit\" value=\"Upload\">\n" );
//      page->SendString( "      </form><br/>\n" );

      page->SendString( "<pre>\n" );
      Config.Show( page );
      ProtocolARP::Show( page );
      osThread::Show( page );
      osEvent::Show( page );
      osQueue::Show( page );
      ProtocolTCP::Show( page );
      page->SendString( "<pre/>\n" );

      page->SendString( "   <body/>\n" );
      page->SendString( "<html/>\n" );
   }
   else if( !strcasecmp( url, "/files/test1.zip" ) )
   {
      page->SendFile( "c:\\test.rar" );
   }
   else if( !strcasecmp( url, "/test/uploadfile" ) )
   {
      printf( "Reading %d bytes\n", page->ContentLength );
      for( int i = 0; i<page->ContentLength; i++ )
      {
         page->Connection->Read();
      }
      printf( "Done reading\n" );
      page->PageStart();
      page->Printf( "Upload %d bytes complete\n", page->ContentLength );
   }
}

//============================================================================
//
//============================================================================

int main( int argc, char* argv[] )
{
   NetworkConfig config;
   config.interfaceNumber = 1;

   printf( "%d bit build\n", (sizeof(void*)==4?32:64) );

   // Start at 1 to skip the file name
   for( int i = 1; i < argc; i++ )
   {
      if( !strcmp( argv[ i ], "-devices" ) )
      {
         PacketIO::DisplayDevices();
         return 0;
      }
      else if( !strcmp( argv[ i ], "-use" ) )
      {
         config.interfaceNumber = atoi( argv[ ++i ] );
      }
      else
      {
         printf( "unknown option '%s'\n", argv[ i ] );
         return -1;
      }
   }

   HTTPD::RegisterPageHandler( ProcessPageRequest );
   MainEntry( &config );

   ProtocolDHCP::test();

   while( 1 )
   {
#ifdef _WIN32
      Sleep( 100 );
#elif __linux__
      usleep( 100000 );
#endif
      ProtocolTCP::Tick();
   }

   return 0;
}


