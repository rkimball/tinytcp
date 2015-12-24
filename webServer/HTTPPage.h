//----------------------------------------------------------------------------
//  Copyright 2015 Robert Kimball
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

#ifndef HTTPPAGE_H
#define HTTPPAGE_H

#include "ProtocolTCP.h"
#include "osThread.h"
#include "osPrintfInterface.h"

class HTTPPage : public osPrintfInterface
{
   friend class HTTPD;

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
   void Font( int size );
   void FontEnd();
   void PageStart( const char* mimeType="text/html" );
   void PageNotFound( void );
   void PageNoContent( void );
   void PageUnauthorized( void );
   void SetRefresh( int interval, const char* title=0 );
   void SetTitle( const char* title );
   void Reference( const char* link, const char* text );
   void Reference( HTTPPage& page, const char* text );
   void Center();
   void CenterEnd();
   void Background( unsigned char red, unsigned char green, unsigned char blue );
   void TableStart( const char* title, int columns, ... );
   void TableStart( bool border, int pad, int space, int percentWidth );
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

   void TableRowStart();
   void TableRowStop();
   void TableRowHeader( HALIGN ha, VALIGN va, int colSpan, const char* value );
   void TableRowData
      (
         HALIGN      ha,
         VALIGN      va,
         int         colSpan,
         const char* fmt, ...
      );
   void TableData( const char* fmt, ... );
   void TableDataStart();
   void TableDataStop();
   void TableHeader( const char* fmt, ... );
   void TableHeaderStart();
   void TableHeaderStop();

   void SelectStart( const char* msg, const char* name, size_t size );
   void SelectAddItem( const char* item, bool selected = false );
   void SelectEnd();

   void RadioAddItem( const char* name, const char* value, const char* displayed, bool checked );
   void Redirect( const char* url, int delay, const char* greeting );
   
   void HorizontalLine( void );

   void FormStart( const char* action );
   void FormTextField( const char* tag, size_t size, const char* initValue = 0 );
   void FormTextField( const char* tag, size_t size, int initValue );
   void FormButton( const char* label );
   void FormCheckboxField( const char* tag, const char* title, bool checked );
   void FormEnd( void );

   void Background( const char* image );

   bool Image( const char* image, int width, int height );

   bool Preformatted( bool enable );

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
   HTTPPage( HTTPPage& );
   HTTPPage();
   ~HTTPPage();

   char        argvArray[400];

   int         ColumnCount;
   bool        DataPreformatted;
   HALIGN      HAlign;
   VALIGN      VAlign;

   osThread    Thread;
};
      
#endif
