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
http::Server WebServer;

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

typedef void(*PageFunction)( http::Page* );

//void MasterPage( http::Page* page, PageFunction content )
//{
//   page->PageOK();
//   page->WriteStartTag( http::Page::html );
//   page->WriteStartTag( http::Page::head );
//   page->WriteTag( http::Page::title, "Protocol Stack Test Page" );

//   page->WriteTag( http::Page::Comment, "Latest compiled and minified CSS" );
//   page->WriteNode( "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css\" integrity=\"sha384-1q8mTJOASx8j1Au+a5WDVnPi2lkFfwwEAa8hDDdjZlpLegxhjVME1fgjWPGmkzs7\" crossorigin=\"anonymous\">" );

//   page->WriteTag( http::Page::Comment, "Optional theme" );
//   page->WriteNode( "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap-theme.min.css\" integrity=\"sha384-fLW2N01lMqjakBkx3l/M9EahuwpSfeNvV63J5ezn3uZzapT0u7EYsXMjQV+0En5r\" crossorigin=\"anonymous\">" );

//   page->WriteTag( http::Page::Comment, "Latest compiled and minified JavaScript" );
//   page->WriteNode( "<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/js/bootstrap.min.js\" integrity=\"sha384-0mSbJDEHialfmuBBQP6A4Qrprq5OVfW37PRR3j5ELqxss1yVqOtnepnHVP9aJ7xS\" crossorigin=\"anonymous\"></script>" );

//   page->WriteEndTag( http::Page::head );
//   page->WriteStartTag( http::Page::body );

//   page->WriteStartTag( "nav" ); page->WriteAttribute( "class", "navbar navbar-default" );
//   page->WriteStartTag( "div" ); page->WriteAttribute( "class", "container-fluid" );
//   page->WriteStartTag( "div" ); page->WriteAttribute( "class", "navbar-header" );
////   page->WriteTag( "button" ); page->WriteAttribute( "type", "button" ); page->WriteAttribute( "class", "navbar-toggle collapsed" );
////   page->WriteAttribute( "data-toggle", "collapse" );
////   page->WriteAttribute( "data-target", "#navbar" );
////   page->WriteAttribute( "aria-expanded", "false" );
////   page->WriteAttribute( "aria-controls", "navbar" );

////           <span class="sr-only">Toggle navigation</span>
////           <span class="icon-bar"></span>
////           <span class="icon-bar"></span>
////           <span class="icon-bar"></span>
////         </button>
////       <a class="navbar-brand" href="#">Project name</a>
//   page->WriteStartTag( "a" ); page->WriteAttribute( "class", "navbar-brand" ); page->WriteAttribute( "href", "#" );
//   page->WriteValue( "tinytcp" );
//   page->WriteEndTag( "a" );
////       </div>
//   page->WriteEndTag( "div" );
////       <div id="navbar" class="navbar-collapse collapse">
//   page->WriteStartTag( "div" ); page->WriteAttribute( "id", "navbar" ); page->WriteAttribute( "class", "navbar-collapse collapse" );
////         <ul class="nav navbar-nav">
//   page->WriteStartTag( "ul" ); page->WriteAttribute( "class", "nav navbar-nav" );
////           <li class="active"><a href="#">Home</a></li>
//   page->WriteStartTag( "li" ); page->WriteStartTag( "a" ); page->WriteAttribute( "href", "#" );
//   page->WriteEndTag("a"); page->WriteEndTag( "li" );
////           <li><a href="#">About</a></li>
////           <li><a href="#">Contact</a></li>
////           <li class="dropdown">
////             <a href="#" class="dropdown-toggle" data-toggle="dropdown" role="button" aria-haspopup="true" aria-expanded="false">Dropdown <span class="caret"></span></a>
////             <ul class="dropdown-menu">
////               <li><a href="#">Action</a></li>
////               <li><a href="#">Another action</a></li>
////               <li><a href="#">Something else here</a></li>
////               <li role="separator" class="divider"></li>
////               <li class="dropdown-header">Nav header</li>
////               <li><a href="#">Separated link</a></li>
////               <li><a href="#">One more separated link</a></li>
////             </ul>
////           </li>
////         </ul>
////         <ul class="nav navbar-nav navbar-right">
////           <li class="active"><a href="./">Default <span class="sr-only">(current)</span></a></li>
////           <li><a href="../navbar-static-top/">Static top</a></li>
////           <li><a href="../navbar-fixed-top/">Fixed top</a></li>
////         </ul>
////       </div><!--/.nav-collapse -->
////     </div><!--/.container-fluid -->
////   </nav>




//   content( page );
//   page->WriteEndTag( http::Page::body );
//}

void HomePage( http::Page* page )
{
   time_t t = time(0);
   struct tm* now = localtime( &t );
   page->Printf( "Current time: %s\n", asctime( now ) );

   page->Printf( "<form action=\"/formsdemo.html\">" );

   page->Printf( "First name:" );
   page->Printf( "<input type=\"text\" name=\"FirstName\" value=\"Mike\"/>" );
   page->Printf( "<br>" );

   page->Printf( "Last name:" );
   page->Printf( "<input type=\"text\" name=\"LastName\" value=\"Easter\"/>" );
   page->Printf( "<br>" );

   page->Printf( "<input type=\"submit\" value=\"submit\" />" );

//      page->Reference( "/files/test1.zip", "test1.zip" );
//      page->SendString( "      <form action=\"/test/uploadfile\" method=\"POST\" " );
//      page->SendString( "      enctype=\"multipart/form-data\" action=\"_URL_\">\n" );
//      page->SendString( "File: <input type=\"file\" name=\"file\" size=\"50\"><br>\n" );
//      page->SendString( "      <input type=\"submit\" value=\"Upload\">\n" );
//      page->SendString( "      </form><br>\n" );

   page->Printf( "<pre>" );
   Config.Show( page );
   ProtocolARP::Show( page );
   osThread::Show( page );
   osEvent::Show( page );
   osQueue::Show( page );
   ProtocolTCP::Show( page );
   page->Printf( "</pre>" );
}

void FormsResponse( http::Page* page )
{
   for( int i=0; i<page->argc; i++ )
   {
      char* name;
      char* value;
      page->ParseArg( page->argv[i], &name, &value );
      page->Printf( "<span>%s = %s</span>", name, value );
      page->Printf( "<br>" );
   }
}

//============================================================================
//
//============================================================================

void ProcessPageRequest
(
   http::Page* page,
   const char* url,
   int argc,
   char** argv
)
{
   printf( "url '%s'\n", url );
   for( int i=0; i<argc; i++ )
   {
      printf( "   arg[%d] = '%s'\n", i, argv[i] );
   }
   if( !strcasecmp( url, "/" ) )
   {
      page->Process( "master.html", "@Content", HomePage );
   }
   else if( !strcasecmp( url, "/formsdemo.html" ) )
   {
      page->Process( "master.html", "@Content", FormsResponse );
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
      page->PageOK();
      page->Printf( "Upload %d bytes complete\n", page->ContentLength );
   }
   else
   {
      page->PageNotFound();
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

   http::Server::RegisterPageHandler( ProcessPageRequest );
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


