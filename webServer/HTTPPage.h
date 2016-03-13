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

#ifndef HTTPPAGE_H
#define HTTPPAGE_H

#include "ProtocolTCP.h"
#include "osThread.h"
#include "osPrintfInterface.h"

namespace http
{
   class Page;
}

class http::Page : public osPrintfInterface
{
   friend class Server;

public:
   static const uint32_t  BUFFER_SIZE = 512;

   int Printf( const char* format, ... );
   static void HTMLEncodef( osPrintfInterface*, const char* format, ... );

   bool Puts( const char* string );

   void Initialize( ProtocolTCP::Connection* );

   bool SendString( const char* string );
   void DumpData( const char* buffer, size_t length );
   bool RawSend( const void* buffer, size_t length );
   void SendASCIIString( const char* string );
   bool SendFile( const char* filename );
   void FontBegin( int size );
   void FontEnd();
   void PageBegin( const char* mimeType="text/html" );
   void PageNotFound( void );
   void PageNoContent( void );
   void PageUnauthorized( void );
   void SetRefresh( int interval, const char* title=0 );
   void SetTitle( const char* title );
   void Reference( const char* link, const char* text );
   void Reference( Page& page, const char* text );
   void CenterBegin();
   void CenterEnd();
   void Background( unsigned char red, unsigned char green, unsigned char blue );
   void TableBegin( const char* title, int columns, ... );
   void TableBegin( bool border, int pad, int space, int percentWidth );
   void TableRow( const char* row1, ... );
   void TableEnd( void );
   typedef enum
   {
      LEFT,
      CENTER,
      RIGHT
   } HALIGN;
   typedef enum
   {
      TOP,
      MIDDLE,
      BOTTOM
   } VALIGN;

   void TableSetHAlign( HALIGN ha );
   void TableSetVAlign( VALIGN va );
   void TableSetColSpan( int colSpan );

   void TableRowBegin();
   void TableRowEnd();
   void TableRowHeader( HALIGN ha, VALIGN va, int colSpan, const char* value );
   void TableRowData
      (
         HALIGN      ha,
         VALIGN      va,
         int         colSpan,
         const char* fmt, ...
      );
   void TableData( const char* fmt, ... );
   void TableDataBegin();
   void TableDataEnd();
   void TableHeader( const char* fmt, ... );
   void TableHeaderBegin();
   void TableHeaderEnd();

   void SelectBegin( const char* msg, const char* name, size_t size );
   void SelectAddItem( const char* item, bool selected = false );
   void SelectEnd();

   void RadioAddItem( const char* name, const char* value, const char* displayed, bool checked );
   void Redirect( const char* url, int delay, const char* greeting );
   
   void HorizontalLine( void );

   void FormBegin( const char* action );
   void FormTextField( const char* tag, size_t size, const char* initValue = 0 );
   void FormTextField( const char* tag, size_t size, int initValue );
   void FormButton( const char* label );
   void FormCheckboxField( const char* tag, const char* title, bool checked );
   void FormEnd( void );

   void Background( const char* image );

   bool Image( const char* image, int width, int height );

   void PreformattedBegin();
   void PreformattedEnd();

   void Flush();

   char*       Directory;
   bool        TableBorder;

   char        URL[ 256 ];
   char        ContentType[ 256 ];
   int         ContentLength;
   int         Busy;
   int         argc;
   char*       argv[ 10 ];

   ProtocolTCP::Connection*   Connection;

private:
   Page( Page& );
   Page();
   ~Page();

   char        argvArray[400];

   int         ColumnCount;
   bool        DataPreformatted;
   HALIGN      HAlign;
   VALIGN      VAlign;

   osThread    Thread;
};
      
#endif
