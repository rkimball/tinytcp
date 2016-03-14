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
   typedef void(*MarkerContent)( http::Page* );

   typedef enum
   {
      Comment,
      Doctype,
      a,
      abbr,
      acronym,
      address,
      applet,
      area,
      article,
      aside,
      audio,
      b,
      base,
      basefont,
      bdi,
      bdo,
      big,
      blockquote,
      body,
      br,
      button,
      canvas,
      caption,
      center,
      cite,
      code,
      col,
      colgroup,
      datalist,
      dd,
      del,
      details,
      dfn,
      dialog,
      dir,
      div,
      dl,
      dt,
      em,
      embed,
      fieldset,
      figcaption,
      figure,
      font,
      footer,
      form,
      frame,
      frameset,
      h1,
      head,
      header,
      hr,
      html,
      i,
      iframe,
      img,
      input,
      ins,
      kbd,
      keygen,
      label,
      legend,
      li,
      link,
      main,
      map,
      mark,
      menu,
      menuitem,
      meta,
      meter,
      nav,
      noframes,
      noscript,
      object,
      ol,
      optgroup,
      option,
      output,
      p,
      param,
      pre,
      progress,
      q,
      rp,
      rt,
      ruby,
      s,
      samp,
      script,
      section,
      select,
      small,
      source,
      span,
      strike,
      strong,
      style,
      sub,
      summary,
      sup,
      table,
      tbody,
      td,
      textarea,
      tfoot,
      th,
      thead,
      time,
      title,
      tr,
      track,
      tt,
      u,
      ul,
      var,
      video,
      wbr
   }TagType;

   class Tag
   {
   public:
      Tag();
      ~Tag();
      Tag& Class( const char* value );
      Tag& Id( const char* value );
   };

   void Initialize( ProtocolTCP::Connection* );

   int Printf( const char* format, ... );
   static void HTMLEncodef( osPrintfInterface*, const char* format, ... );

   /// Puts writes the string converting all newline characters to <br/>
   bool Puts( const char* string );
   bool SendString( const char* string );
   bool RawSend( const void* buffer, size_t length );
   /// SendASCIIString converts all non-displayable characters
   /// to '%XX' where XX is the hex values of the character
   void SendASCIIString( const char* string );
   void DumpData( const char* buffer, size_t length );
   bool SendFile( const char* filename );

   void PageOK( const char* mimeType="text/html" );
   void PageNotFound();
   void PageNoContent();
   void PageUnauthorized();

   void WriteStartTag( http::Page::TagType );
   void WriteStartTag( const char* tag );
   void WriteTag( http::Page::TagType, const char* value=NULL );
   void WriteEndTag( http::Page::TagType );
   void WriteEndTag( const char* tag );
   void WriteAttribute( const char* name, const char* value );
   void WriteValue( const char* value );
   void WriteNode( const char* value );

   void ParseArg( char* arg, char** name, char** value );

   void Process( const char* htmlFile, const char* marker, MarkerContent );

   void Flush();

   char*       Directory;
   bool        TableBorder;

   char        URL[ 256 ];
   char        ContentType[ 256 ];
   int         ContentLength;
   int         Busy;
   int         argc;
   char*       argv[ 10 ];
   int         TagDepth;
   bool        StartTagOpen;

   ProtocolTCP::Connection*   Connection;

private:
   Page( Page& );
   Page();
   ~Page();

   const char* TagTypeToString( TagType tag );
   void ClosePendingOpen();

   int         ColumnCount;
   bool        DataPreformatted;

   osThread    Thread;
};
      
#endif
