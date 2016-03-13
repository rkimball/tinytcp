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

http::Page::Page()
{
}

http::Page::~Page()
{
}

void http::Page::Initialize( ProtocolTCP::Connection* connection )
{
   Connection = connection;
   ColumnCount = 0;
   DataPreformatted = false;
   TableBorder = true;
   Busy = 0;
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

void http::Page::Background( unsigned char red, unsigned char green, unsigned char blue )
{
   char        buffer[ BUFFER_SIZE ];

   snprintf( buffer, sizeof(buffer), "<body bgcolor=\"%02X%02X%02X\">\n", red, green, blue );
   SendString( buffer );
}

//============================================================================
//
//============================================================================

void http::Page::FontBegin( int size )
{
   char        buffer[ BUFFER_SIZE ];

   snprintf( buffer, sizeof(buffer), "<font size=\"%d\">\n", size );
   SendString( buffer );
}

//============================================================================
//
//============================================================================

void http::Page::FontEnd()
{
   SendString( "</font>" );
}

//============================================================================
//
//============================================================================

void http::Page::CenterBegin()
{
   SendString( "<center>" );
}

//============================================================================
//
//============================================================================

void http::Page::CenterEnd()
{
   SendString( "</center>" );
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

void http::Page::PageBegin( const char* mimeType )
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

void http::Page::SetRefresh( int interval, const char* title )
{
   SendString( "<head>\n" );
   if( title != 0 )
   {
      SendString( "   <title>" );
      SendString( title );
      SendString( "</title>\n" );
   }
   Printf( "   <META HTTP-EQUIV=\"REFRESH\" CONTENT=\"%d\">", interval );
   SendString( "</head>\n" );
}

//============================================================================
//
//============================================================================

void http::Page::SetTitle( const char* title )
{
   SendString( "<title>" );
   SendString( title );
   SendString( "</title>\n" );
}

//============================================================================
//
//============================================================================

void http::Page::Reference( const char* link, const char* text )
{
   if( *link == '/' || !strncasecmp( (char*)link, "http:", 5 ) )
   {
      SendString( "<a href=\"" );
      SendASCIIString( link );
      SendString( "\">" );
      SendString( text );
      SendString( "</a>" );
   }
   else
   {
      SendString( "<a href=\"/" );
      SendString( Directory );
      SendString( "/" );
      SendASCIIString( link );
      SendString( "\">" );
      SendString( text );
      SendString( "</a>" );
   }
}

//============================================================================
//
//============================================================================

void http::Page::Reference( Page& page, const char* text )
{
   char        s[ BUFFER_SIZE ];

   snprintf( s, sizeof(s), "<a href=\"/%s/\">%s</a>", page.Directory, text );
   SendString( s );
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

   vsnprintf( buffer, sizeof(buffer), format, vlist );   //lint !e530
   
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

   vsnprintf( buffer, sizeof(buffer), format, vlist );   //lint !e530

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

void http::Page::TableBegin( bool border, int pad, int space, int percentWidth )
{
   Printf
   (
      "<table border=\"%d\" cellpadding=\"%d\" cellspacing=\"%d\" ",
      (border?1:0),
      pad,
      space
   );
   if( percentWidth > 0 )
   {
      Printf( "width=\"%d%%\"", percentWidth );
   }
   SendString( ">\n" );
}

//============================================================================
//
//============================================================================

void http::Page::TableRowBegin()
{
   SendString( "<tr>\n" );
}

//============================================================================
//
//============================================================================

void http::Page::TableRowEnd()
{
   SendString( "</tr>\n" );
}

//============================================================================
//
//============================================================================

void http::Page::TableRowHeader( HALIGN ha, VALIGN va, int colSpan, const char* value )
{
   char        buffer[ BUFFER_SIZE ];
   const char* horiz="";
   const char* vert="";

   switch( ha )
   {
   case LEFT:
      horiz = "left";
      break;
   case CENTER:
      horiz = "center";
      break;
   case RIGHT:
      horiz = "right";
      break;
   }

   switch( va )
   {
   case TOP:
      vert = "top";
      break;
   case MIDDLE:
      vert = "middle";
      break;
   case BOTTOM:
      vert = "bottom";
      break;
   }

   SendString( "   <th style=\"padding:0.1em 2em 0.1em 2em;\" " );
   snprintf
   (
      buffer,
      sizeof(buffer),
      "align=\"%s\" valign=\"%s\" colspan=\"%d\">\n      ",
      horiz,
      vert,
      colSpan
   );
   SendString( buffer );
   SendString( value );
   SendString( "\n   </th>\n" );
}

//============================================================================
//
//============================================================================

void http::Page::TableRowData
(
   HALIGN      ha,
   VALIGN      va,
   int         colSpan,
   const char* fmt, ...
)
{
   char        buffer[ BUFFER_SIZE ];
   const char* horiz="";
   const char* vert="";
   va_list     vlist;

   switch( ha )
   {
   case LEFT:
      horiz = "left";
      break;
   case CENTER:
      horiz = "center";
      break;
   case RIGHT:
      horiz = "right";
      break;
   }

   switch( va )
   {
   case TOP:
      vert = "top";
      break;
   case MIDDLE:
      vert = "middle";
      break;
   case BOTTOM:
      vert = "bottom";
      break;
   }

   SendString( "   <td " );
   SendString( "style=\"padding:0.1em 2em 0.1em 2em;\" " );
   snprintf
   (
      buffer,
      sizeof(buffer),
      "align=\"%s\" valign=\"%s\" colspan=\"%d\">\n      ",
      horiz,
      vert,
      colSpan
   );
   SendString( buffer );

   va_start( vlist, fmt );
   vsnprintf( buffer, sizeof(buffer), fmt, vlist );   //lint !e530
   va_end( vlist );

   SendString( buffer );

   SendString( "\n   </td>\n" );
}

//============================================================================
//
//============================================================================

void http::Page::TableBegin( const char* title, int columns, ... )
{
   char        buffer[ BUFFER_SIZE ];
   int         i;
   va_list     vlist;

   va_start( vlist, columns );

   ColumnCount = columns;

   snprintf( buffer, sizeof(buffer), "<table border=\"%d\">\n",(TableBorder?1:0) );
   SendString( buffer );

   if( title != 0 )
   {
      snprintf( buffer, sizeof(buffer), "<tr>\n<th colspan=\"%d\">%s</th>\n</tr>", columns, title );
      SendString( buffer );
   }

   SendString( "<tr>\n" );
   for( i=0; i<columns; i++ )
   {
      snprintf( buffer, sizeof(buffer), "<th>%s</th>\n", va_arg( vlist, char* ) );
      SendString( buffer );
   }
   SendString( "</tr>\n" );

   va_end( vlist );
}

//============================================================================
//
//============================================================================

void http::Page::TableRow( const char* column1, ... )
{
   char        s[ BUFFER_SIZE ];
   int         i;
   const char* string;
   va_list     vlist;

   va_start( vlist, column1 );
   string = column1;

   SendString( "<tr>\n" );
   for( i=0; i<ColumnCount; i++ )
   {
      snprintf( s,sizeof(s), "<td>%s</td>", string );
      SendString( s );
      string = (char*)va_arg( vlist, char* );
   }
   SendString( "</tr>\n" );

   va_end( vlist );
}

//============================================================================
//
//============================================================================

void http::Page::TableEnd( void )
{
   const char* s = "</table>\n";
   SendString( s );
}

//============================================================================
//
//============================================================================

void http::Page::TableDataBegin()
{
   SendString( "<td>" );
}

//============================================================================
//
//============================================================================

void http::Page::TableDataEnd()
{
   SendString( "</td>\n" );
}

//============================================================================
//
//============================================================================

void http::Page::TableHeaderBegin()
{
   SendString( "<th>" );
}

//============================================================================
//
//============================================================================

void http::Page::TableHeaderEnd()
{
   SendString( "</th>\n" );
}

//============================================================================
//
//============================================================================

void http::Page::TableSetHAlign( Page::HALIGN ha )
{
}

//============================================================================
//
//============================================================================

void http::Page::TableSetVAlign( Page::VALIGN va )
{
}

//============================================================================
//
//============================================================================

void http::Page::TableSetColSpan( int colSpan )
{
}

//============================================================================
//
//============================================================================

void http::Page::TableHeader( const char* fmt, ... )
{
   char        buffer[ 256 ];
   va_list     vlist;

   va_start( vlist, fmt );

   vsnprintf( buffer, sizeof(buffer), fmt, vlist );   //lint !e530
   SendString( "<th>" );
   SendString( buffer );
   SendString( "</th>" );
}

//============================================================================
//
//============================================================================

void http::Page::TableData( const char* fmt, ... )
{
   char        buffer[ 256 ];
   va_list     vlist;

   va_start( vlist, fmt );

   vsnprintf( buffer, sizeof(buffer), fmt, vlist );   //lint !e530
   SendString( "<td>" );
   SendString( buffer );
   SendString( "</td>" );
}

//============================================================================
//
//============================================================================

void http::Page::FormBegin( const char* action )
{
   char     s[ BUFFER_SIZE ];

   snprintf( s, sizeof(s), "<form action=\"/%s/%s\" method=get>\n<div>\n", Directory, action );
   SendString( s );
}

//============================================================================
//
//============================================================================

void http::Page::FormTextField( const char* tag, size_t size, const char* initValue )
{
   char     s[ BUFFER_SIZE ];

   if( initValue )
   {
      snprintf( s, sizeof(s), "<input type=text name=\"%s\" size=\"%d\" value=\"%s\">\n",
               tag, (int)size, initValue );
   }
   else
   {
      snprintf( s, sizeof(s), "<input type=text name=\"%s\" size=\"%d\">\n", tag, (int)size );
   }
   SendString( s );
}

//============================================================================
//
//============================================================================

void http::Page::FormTextField( const char* tag, size_t size, int initValue )
{
   char  s[20];

   snprintf( s, sizeof(s), "%d", initValue );

   FormTextField( tag, size, s );
}

//============================================================================
//
//============================================================================

void http::Page::FormCheckboxField( const char* tag, const char* title, bool checked )
{
   char     s[ BUFFER_SIZE ];

   if( checked )
   {
      snprintf( s, sizeof(s), "<input type=\"checkbox\" name=\"%s\" Checked> %s\n", tag, title );
   }
   else
   {
      snprintf( s, sizeof(s), "<input type=\"checkbox\" name=\"%s\"> %s\n", tag, title );
   }
   SendString( s );
}

//============================================================================
//
//============================================================================

void http::Page::FormButton( const char* label )
{
   char     s[ BUFFER_SIZE ];

   snprintf( s, sizeof(s), "<input type=\"submit\" value=\"%s\">\n", label );
   SendString( s );
}

//============================================================================
//
//============================================================================

void http::Page::FormEnd( void )
{
   SendString( "</div>\n</form>\n" );
}

//============================================================================
//
//============================================================================

void http::Page::HorizontalLine( void )
{
   SendString( "<hr/>\n" );
}

//============================================================================
//
//============================================================================

bool http::Page::Image( const char* image, int width, int height )
{
   char     s[ BUFFER_SIZE ];

   snprintf
   (
      s,
      sizeof(s),
      "<img src=\"/%s/%s\" height=\"%d\" width=\"%d\">\n",
      Directory,
      image,
      height,
      width
   );
   return SendString( s );
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

void http::Page::Background( const char* image )
{
   char     s[ BUFFER_SIZE ];
   snprintf( s, sizeof(s), "<body background=\"/%s/%s\">", Directory, image );
   SendString( s );
}

//============================================================================
//
//============================================================================

void http::Page::PreformattedBegin()
{
   SendString( "<pre>\n" );
}

//============================================================================
//
//============================================================================

void http::Page::PreformattedEnd()
{
   SendString( "</pre>\n" );
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

bool http::Page::Puts( const char* string )
{
   int      i;
   int      length = (int)strlen(string);

   if( DataPreformatted )
   {
      RawSend( string, strlen(string) );
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

bool http::Page::RawSend( const void* p, size_t length )
{
   Connection->Write( (uint8_t*)p, length );

   return true;
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

void http::Page::SelectBegin( const char* msg, const char* name, size_t size )
{
   char     s[ BUFFER_SIZE ];
   snprintf( s, sizeof(s),"%s<select name=\"%s\" size=\"%lud\">",msg,name,size);
   SendString( s );
}

//============================================================================
//
//============================================================================

void http::Page::SelectAddItem( const char* item, bool selected )
{
   char     s[ BUFFER_SIZE ];
   if( selected == false )
   {
      snprintf(s, sizeof(s),"<option>%s",item);
   }
   else
   {
      snprintf(s, sizeof(s),"<option selected>%s",item);
   }
   SendString( s );
}

//============================================================================
//
//============================================================================

void http::Page::SelectEnd()
{
   SendString("</select>");
}

//============================================================================
//
//============================================================================

void http::Page::RadioAddItem( const char* name, const char* value, const char* displayed, bool checked )
{
   if( checked )
   {
      SendString( "<input type=\"radio\" checked name=\"" );
   }
   else
   {
      SendString( "<input type=\"radio\" name=\"" );
   }

   SendString( name );
   SendString( "\" value=\"" );
   SendString( value );
   SendString( "\"> " );
   SendString( displayed );
}

//============================================================================
//
//============================================================================

void http::Page::Redirect( const char* url, int delay, const char* greeting )
{
   // url      - can be relative
   // delay    - in seconds
   // greeting - message to display for "delay" seconds

   char  s[ BUFFER_SIZE ];
   snprintf(s, sizeof(s),"<meta http-equiv=\"Refresh\" content=\"%d;URL=http:%s\">",delay,url);
   if( SendString( s ) )
   {
      snprintf(s, sizeof(s),"%s",greeting);
      SendString( s );
   }
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

