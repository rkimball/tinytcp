//----------------------------------------------------------------------------
//  Copyright 2015  Robert Kimball
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http ://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//----------------------------------------------------------------------------

#include <stdio.h>
#include <tchar.h>
#include <pcap.h>

#include "PacketIO.h"
#include "ProtocolMACEthernet.h"
#include "Address.h"
#include "ProtocolTCP.h"
#include "osThread.h"
#include "osMutex.h"
#include "HTTPD.h"
#include "HTTPPage.h"
#include "NetworkInterface.h"

AddressConfiguration Config;

PacketIO* PIO;
static osThread NetworkThread;
static osThread MainThread;
static osMutex* Semaphore;
HTTPD WebServer;

extern DataBuffer TxBuffer[ TX_BUFFER_COUNT ];
extern DataBuffer RxBuffer[ RX_BUFFER_COUNT ];

struct NetworkConfig
{
   int interfaceNumber;
};

//============================================================================
// Callback function invoked by libpcap for every incoming packet
//============================================================================

void packet_handler( u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data )
{
   ProtocolMACEthernet::ProcessRx( (uint8_t*)pkt_data, header->len );
}

//============================================================================
//
//============================================================================

void NetworkEntry( void* param )
{
   NetworkConfig& config = *(NetworkConfig*)param;
   char device[ 256 ];

   Config.Address.Hardware[ 0 ] = 0x00;
   Config.Address.Hardware[ 1 ] = 0xA0;
   Config.Address.Hardware[ 2 ] = 0xC6;
   Config.Address.Hardware[ 3 ] = 0x00;
   Config.Address.Hardware[ 4 ] = 0x0B;
   Config.Address.Hardware[ 5 ] = 0x0B;

   // 192.168.1.3
   Config.Address.Protocol[ 0 ] = 192;
   Config.Address.Protocol[ 1 ] = 168;
   Config.Address.Protocol[ 2 ] = 1;
   Config.Address.Protocol[ 3 ] = 3;

   Config.SubnetMask[ 0 ] = 255;
   Config.SubnetMask[ 1 ] = 255;
   Config.SubnetMask[ 2 ] = 254;
   Config.SubnetMask[ 3 ] = 0;

   Config.Gateway[ 0 ] = 129;
   Config.Gateway[ 1 ] = 46;
   Config.Gateway[ 2 ] = 72;
   Config.Gateway[ 3 ] = 1;

   PacketIO::GetDevice( config.interfaceNumber, device, sizeof( device ) );
   printf( "using device %s\n", device );
   PacketIO::GetMACAddress( device, Config.Address.Hardware );

   PIO = new PacketIO( device );

   ProtocolMACEthernet::Initialize( PIO );

   // This method does not return...ever
   PIO->Start( packet_handler );
}

//============================================================================
//
//============================================================================

void MainEntry( void* config )
{
   NetworkThread.Create( NetworkEntry, "Network", 1024, 10, config );

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
   printf( "url %s\n", url );

   if( !_stricmp( url, "/" ) )
   {
      page->PageStart();
      page->SendString( "<html>\n" );
      page->SendString( "   <head>\n" );
      page->SendString( "      <title>Protocol Stack Test Page</title>\n" );
      page->SendString( "   </head>\n" );
      page->SendString( "   <body>\n" );
      page->Printf( "Hello World!\n" );
      page->Reference( "/files/test1.zip", "test1.zip" );

      page->SendString( "      <form action=\"/test/uploadfile\" method=\"POST\" " );
      page->SendString( "      enctype=\"multipart/form-data\" action=\"_URL_\">\n" );
      page->SendString( "File: <input type=\"file\" name=\"file\" size=\"50\"><br/>\n" );
      page->SendString( "      <input type=\"submit\" value=\"Upload\">\n" );
      page->SendString( "      </form><br/>\n" );
      page->SendString( "   <body/>\n" );

      page->SendString( "<pre>\n" );
      osThread::Show( page );
      osEvent::Show( page );
      ProtocolTCP::Show( page );
      page->SendString( "<pre/>\n" );

      page->SendString( "<html/>\n" );

   }
   else if( !_stricmp( url, "/files/test1.zip" ) )
   {
      page->SendFile( "c:\\test.rar" );
   }
   else if( !_stricmp( url, "/test/uploadfile" ) )
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
   }
   //crc_test();
   //MainThread.Create( MainEntry, "Main", 1024, 20, 0 );
   HTTPD::RegisterPageHandler( ProcessPageRequest );
   MainEntry( &config );

   while( 1 )
   {
      Sleep( 100 );
      ProtocolTCP::Tick();
   }

   return 0;
}


