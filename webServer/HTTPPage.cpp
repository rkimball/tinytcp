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
#include <stdarg.h>
#include <string.h>

#include "HTTPPage.h"
#include "osThread.h"

#ifdef WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#elif __linux__
#include <strings.h>
#endif

http::Page::Page() :
   TagDepth(0),
   StartTagOpen(false)
{
}

http::Page::~Page()
{
}

void http::Page::Initialize( ProtocolTCP::Connection* connection )
{
   Connection = connection;
   DataPreformatted = false;
   TableBorder = true;
   Busy = 0;
}

//============================================================================
//
//============================================================================

int http::Page::Printf( const char* format, ... )
{
   char        buffer[ BUFFER_SIZE ];
   int         i;
   va_list     vlist;
   int         rc;

   va_start( vlist, format );

   vsnprintf( buffer, sizeof(buffer), format, vlist );

   if( DataPreformatted )
   {
      SendString( buffer );
      rc = (int)strlen(buffer);
   }
   else
   {
      for( i=0; buffer[i] && i<(int)(sizeof(buffer)-1); i++ )
      {
         if( buffer[i] == '\n' )
         {
            SendString( "<br/>" );
         }
         else if( buffer[i] == '\r' )
         {
         }
         else
         {
            RawSend( &buffer[ i ], 1 );
         }
      }
      rc = i;
   }

   return rc;
}

//============================================================================
//
//============================================================================

void http::Page::HTMLEncodef( osPrintfInterface* pfunc, const char* format, ... )
{
   char        buffer[ 256 ];
   char        buffer2[ sizeof(buffer)*2 ];
   char*       p;
   int         i;
   va_list     vlist;

   va_start( vlist, format );

   vsnprintf( buffer, sizeof(buffer), format, vlist );

   p = buffer2;
   for( i=0; buffer[i]; i++ )
   {
      if( p >= &buffer2[ sizeof(buffer2)-5 ] )
      {
         break;
      }
      switch( buffer[ i ] )
      {
      case '<':
         *p++ = '&';
         *p++ = 'l';
         *p++ = 't';
         break;

      case '>':
         *p++ = '&';
         *p++ = 'g';
         *p++ = 't';
         break;

      default:
         *p++ = buffer[ i ];
         break;
      }
   }
   *p = 0;

   pfunc->Printf( buffer2 );
}

//============================================================================
//
//============================================================================

bool http::Page::Puts( const char* string )
{
   int      i;
   int      length = (int)strlen(string);

   if( DataPreformatted )
   {
      RawSend( string, length );
   }
   else
   {
      for( i=0; i<length; i++ )
      {
         switch( string[i] )
         {
         case '\n':
            RawSend( "<br/>", 5 );
            break;
         case '\r':
            break;
         default:
            RawSend( &string[i], 1 );
            break;
         }
      }
   }

   return true;
}

//============================================================================
//
//============================================================================

bool http::Page::SendString( const char* string )
{
   return RawSend( string, (int)strlen(string) );
}

//============================================================================
//
//============================================================================

bool http::Page::RawSend( const void* p, size_t length )
{
   Connection->Write( (uint8_t*)p, length );

   return true;
}

//============================================================================
//
//============================================================================

void http::Page::SendASCIIString( const char* string )
{
   char     buffer[4];

   while( *string )
   {
      if( *string < 0x21 || *string > 0x7E )
      {
         snprintf( buffer, sizeof(buffer), "%%%02X", *string );
         RawSend( buffer, 3 );
      }
      else
      {
         RawSend( string, 1 );
      }
      string++;
   }
}

//============================================================================
//
//============================================================================

void http::Page::DumpData( const char* buffer, size_t length )
{
   int   i;
   int   j;

   i = 0;
   j = 0;
   SendString( "<code>\n" );
   while( i+1 <= length )
   {
      Printf( "%04X ", i );
      for( j=0; j<16; j++ )
      {
         if( i+j < length )
         {
            Printf( "%02X ", buffer[i+j] );
         }
         else
         {
            Printf( "   " );
         }

         if( j == 7 )
         {
            Printf( "- " );
         }
      }
      Printf( "  " );
      for( j=0; j<16; j++ )
      {
         if( buffer[i+j] >= 0x20 && buffer[i+j] <= 0x7E )
         {
            Printf( "%c", buffer[i+j] );
         }
         else
         {
            Printf( "." );
         }
         if( i+j+1 == length )
         {
            break;
         }
      }


      i += 16;

      SendString( "<br/>\n" );
   }
   SendString( "</code>\n" );
}

//============================================================================
//
//============================================================================

void http::Page::PageNotFound( void )
{
   SendString( "HTTP/1.0 404 Not Found\r\n\r\n" );
}

//============================================================================
//
//============================================================================

void http::Page::PageOK( const char* mimeType )
{
   TableBorder = true;
   SendString( "HTTP/1.0 200 OK\r\nContent-type: " );
   SendString( mimeType );
   SendString( "\r\n\r\n" );
}

//============================================================================
//
//============================================================================

void http::Page::PageNoContent( void )
{
   SendString( "HTTP/1.0 204 No Content\r\nContent-type: text/html\r\n\r\n" );
}

//============================================================================
//
//============================================================================

void http::Page::PageUnauthorized( void )
{
   SendString( "HTTP/1.0 401 Unauthorized\r\nContent-type: text/html\r\n\r\n" );
}

//============================================================================
//
//============================================================================

bool http::Page::SendFile( const char* filename )
{
   char     s[ BUFFER_SIZE ];
   FILE*    f;
   bool     rc = false;
   unsigned char     buffer[512];
   uint64_t      counti64;
   int count;

   f = fopen( filename, "rb" );
   if( f )
   {
#ifdef _WIN32
      _fseeki64( f, 0, SEEK_END );
      counti64 = _ftelli64( f );
      _fseeki64( f, 0, SEEK_SET );
#elif __linux__
      fseek( f, 0, SEEK_END );
      counti64 = ftell( f );
      fseek( f, 0, SEEK_SET );
#endif
      SendString( "HTTP/1.0 200 OK\r\nContent-Type: application/java-archive\r\n" );
      snprintf( s, sizeof(s), "Content-Length: %lud\r\n\r\n", counti64 );
      SendString( s );

      do
      {
         count = (int)fread( buffer, 1, sizeof(buffer), f );
#ifdef _WIN32
         uint64_t offset = _ftelli64( f );
#elif __linux__
         uint64_t offset = ftell( f );
#endif
         if( count > 0 )
         {
            RawSend( buffer, count );
         }
      } while( count > 0 );

      rc = true;
   }

   return rc;
}

//============================================================================
//
//============================================================================

void http::Page::Flush()
{
   Connection->Flush();
}

//============================================================================
//
//============================================================================

void http::Page::WriteStartTag( http::Page::TagType type )
{
   WriteStartTag( TagTypeToString(type) );
}

//============================================================================
//
//============================================================================

void http::Page::WriteStartTag( const char* tag )
{
   ClosePendingOpen();
   RawSend( "<", 1 );
   SendString( tag );
   TagDepth++;
   StartTagOpen = true;
}

//============================================================================
//
//============================================================================

void http::Page::WriteTag( http::Page::TagType type, const char* value )
{
   switch( type )
   {
   case Comment:
      ClosePendingOpen();
      RawSend( "<!-- ", 5 );
      if( value ) SendString( value );
      RawSend( " -->", 4 );
      break;
   case br:
      RawSend( "<br>", 4 );
      break;
   default:
      WriteStartTag( type );
      if( value ) WriteValue( value );
      WriteEndTag( type );
      break;
   }
}

//============================================================================
//
//============================================================================

void http::Page::WriteNode( const char* text )
{
   ClosePendingOpen();
   SendString( text );
}

//============================================================================
//
//============================================================================

void http::Page::WriteEndTag( http::Page::TagType type )
{
   WriteEndTag( TagTypeToString(type) );
}

//============================================================================
//
//============================================================================

void http::Page::WriteEndTag( const char* tag )
{
   ClosePendingOpen();
   if( TagDepth > 0 )
   {
      TagDepth--;
      RawSend( "</", 2 );
      SendString( tag );
      RawSend( ">", 1 );
   }
}

//============================================================================
//
//============================================================================

void http::Page::WriteAttribute( const char* name, const char* value )
{
   RawSend( " ", 1 );
   SendString( name );
   RawSend( "=\"", 2 );
   SendString( value );
   RawSend( "\"", 1 );
}

//============================================================================
//
//============================================================================

void http::Page::WriteValue( const char* value )
{
   ClosePendingOpen();
   SendString( value );
}

//============================================================================
//
//============================================================================

void http::Page::ClosePendingOpen()
{
   if( StartTagOpen )
   {
      RawSend( ">", 1 );
      StartTagOpen = false;
   }
}

//============================================================================
//
//============================================================================

void http::Page::ParseArg( char* arg, char** name, char** value )
{
   *name = arg;
   while( *arg )
   {
      if( *arg == '=' )
      {
         *arg = 0;
         *value = ++arg;
         break;
      }
      arg++;
   }
}

//============================================================================
//
//============================================================================

const char* http::Page::TagTypeToString( TagType tag )
{
   const char* rc = NULL;
   switch(tag)
   {
   case Comment: rc = "!--"; break;
   case Doctype: rc = "!DOCTYPE"; break;
   case a: rc = "a"; break;
   case abbr: rc = "abbr"; break;
   case acronym: rc = "acronym"; break;
   case address: rc = "address"; break;
   case applet: rc = "applet"; break;
   case area: rc = "area"; break;
   case article: rc = "article"; break;
   case aside: rc = "aside"; break;
   case audio: rc = "audio"; break;
   case b: rc = "b"; break;
   case base: rc = "base"; break;
   case basefont: rc = "basefont"; break;
   case bdi: rc = "bdi"; break;
   case bdo: rc = "bdo"; break;
   case big: rc = "big"; break;
   case blockquote: rc = "blockquote"; break;
   case body: rc = "body"; break;
   case br: rc = "br"; break;
   case button: rc = "button"; break;
   case canvas: rc = "canvas"; break;
   case caption: rc = "caption"; break;
   case center: rc = "center"; break;
   case cite: rc = "cite"; break;
   case code: rc = "code"; break;
   case col: rc = "col"; break;
   case colgroup: rc = "colgroup"; break;
   case datalist: rc = "datalist"; break;
   case dd: rc = "dd"; break;
   case del: rc = "del"; break;
   case details: rc = "details"; break;
   case dfn: rc = "dfn"; break;
   case dialog: rc = "dialog"; break;
   case dir: rc = "dir"; break;
   case div: rc = "div"; break;
   case dl: rc = "dl"; break;
   case dt: rc = "dt"; break;
   case em: rc = "em"; break;
   case embed: rc = "embed"; break;
   case fieldset: rc = "fieldset"; break;
   case figcaption: rc = "figcaption"; break;
   case figure: rc = "figure"; break;
   case font: rc = "font"; break;
   case footer: rc = "footer"; break;
   case form: rc = "form"; break;
   case frame: rc = "frame"; break;
   case frameset: rc = "frameset"; break;
   case h1: rc = "h1"; break;
   case head: rc = "head"; break;
   case header: rc = "header"; break;
   case hr: rc = "hr"; break;
   case html: rc = "html"; break;
   case i: rc = "i"; break;
   case iframe: rc = "iframe"; break;
   case img: rc = "img"; break;
   case input: rc = "input"; break;
   case ins: rc = "ins"; break;
   case kbd: rc = "kbd"; break;
   case keygen: rc = "keygen"; break;
   case label: rc = "label"; break;
   case legend: rc = "legend"; break;
   case li: rc = "li"; break;
   case link: rc = "link"; break;
   case main: rc = "main"; break;
   case map: rc = "map"; break;
   case mark: rc = "mark"; break;
   case menu: rc = "menu"; break;
   case menuitem: rc = "menuitem"; break;
   case meta: rc = "meta"; break;
   case meter: rc = "meter"; break;
   case nav: rc = "nav"; break;
   case noframes: rc = "noframes"; break;
   case noscript: rc = "noscript"; break;
   case object: rc = "object"; break;
   case ol: rc = "ol"; break;
   case optgroup: rc = "optgroup"; break;
   case option: rc = "option"; break;
   case output: rc = "output"; break;
   case p: rc = "p"; break;
   case param: rc = "param"; break;
   case pre: rc = "pre"; break;
   case progress: rc = "progress"; break;
   case q: rc = "q"; break;
   case rp: rc = "rp"; break;
   case rt: rc = "rt"; break;
   case ruby: rc = "ruby"; break;
   case s: rc = "s"; break;
   case samp: rc = "samp"; break;
   case script: rc = "script"; break;
   case section: rc = "section"; break;
   case select: rc = "select"; break;
   case small: rc = "small"; break;
   case source: rc = "source"; break;
   case span: rc = "span"; break;
   case strike: rc = "strike"; break;
   case strong: rc = "strong"; break;
   case style: rc = "style"; break;
   case sub: rc = "sub"; break;
   case summary: rc = "summary"; break;
   case sup: rc = "sup"; break;
   case table: rc = "table"; break;
   case tbody: rc = "tbody"; break;
   case td: rc = "td"; break;
   case textarea: rc = "textarea"; break;
   case tfoot: rc = "tfoot"; break;
   case th: rc = "th"; break;
   case thead: rc = "thead"; break;
   case time: rc = "time"; break;
   case title: rc = "title"; break;
   case tr: rc = "tr"; break;
   case track: rc = "track"; break;
   case tt: rc = "tt"; break;
   case u: rc = "u"; break;
   case ul: rc = "ul"; break;
   case var: rc = "var"; break;
   case video: rc = "video"; break;
   case wbr: rc = "wbr"; break;
   }
   return rc;
}

//============================================================================
//
//============================================================================

void http::Page::Process( const char* htmlFile, const char* marker, MarkerContent content )
{
   char* path;
   realpath( htmlFile, path );
   FILE* f = fopen( htmlFile, "r" );
   if( f )
   {
      int c;
      int markerIndex = 0;
      int markerLength = strlen(marker);
      while( (c = fgetc( f )) > 0 )
      {
         if( c == marker[markerIndex] )
         {
            markerIndex++;
            if( markerIndex == markerLength )
            {
               // found the marker
               markerIndex = 0;
               SendString( "<!-- " );
               SendString( marker );
               SendString( " start -->" );
               content( this );
               SendString( "<!-- " );
               SendString( marker );
               SendString( " end -->" );
            }
         }
         else if( markerIndex > 0 )
         {
            // Send the part of the marker that matched so far
            RawSend( marker, markerIndex );
            markerIndex = 0;
         }
         else
         {
            char ch = c;
            RawSend( &ch, 1 );
         }
      }
   }
   else
   {
      printf( "failed to open file '%s'\n", htmlFile );
   }
}
