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

#include <stdio.h>
#include <string.h>

#include "ProtocolTCP.h"
#include "HTTPD.h"
#include "HTTPPage.h"
//#include "base64.h"
#include "osThread.h"
#include "osQueue.h"

#ifdef WIN32
#define stricmp _stricmp
#define snprintf _snprintf
#define strdup _strdup
#endif

HTTPPage    HTTPD::PagePoolPages[ MAX_ACTIVE_CONNECTIONS ];
osQueue     HTTPD::PagePool;
PageRequestHandler HTTPD::PageHandler = 0;

#define MAX_ARGV 10

bool                 HTTPD::DebugFlag = false;

static osThread      Thread;

static ProtocolTCP::Connection*  ListenerConnection;
static ProtocolTCP::Connection*  CurrentConnection;

void HTTPD::RegisterPageHandler( PageRequestHandler handler )
{
   PageHandler = handler;
}

void HTTPD::ProcessRequest( HTTPPage* page )
{
   int            i;
   static char    buffer1[ 512 ];
   static char    buffer[ 256 ];
   int            actualSizeRead;
   char*          p;
   char*          result;
   char*          path;
   char*          url = "";
   char           username[20];
   char           password[20];
   int            argc;
   char*          argv[ MAX_ARGV ];
   int            rc;
   ProtocolTCP::Connection* connection = page->Connection;

   actualSizeRead = connection->ReadLine( buffer1, sizeof(buffer1) );
   if( actualSizeRead == -1 )
   {
      return;
   }

   strtok( buffer1, " " );
   path = strtok( 0, " " );

   username[0] = 0;
   password[0] = 0;
   do
   {
      char*    encryptionType;
      char*    loginString;
      actualSizeRead = connection->ReadLine( buffer, sizeof(buffer) );
      if( buffer[0] == 0 )
      {
         break;
      }

      p = strtok( buffer, ":" );
      if( !stricmp( "Authorization", p ) )
      {
         encryptionType = strtok( 0, " " );
         loginString = strtok( 0, " " );
         if( !stricmp( encryptionType, "Basic" ) )
         {
            //char     s[80];
            //char*    tmp;

            //// Good so far...
            //DecodeBase64( loginString, s, sizeof(s) );
            //tmp = tokenizer.strtok( s, ":" );
            //if( tmp != NULL )
            //{
            //   strncpy( username, tmp, sizeof(username)-1 );
            //   tmp = tokenizer.strtok( 0, ":" );
            //   if( tmp != NULL )
            //   {
            //      strncpy( password, tmp, sizeof(password)-1 );
            //   }
            //}
         }
      }
      else if( !stricmp( "Content-Type", p ) )
      {
         char* tmp = strtok( 0, "" );
         if( tmp != NULL )
         {
            //strncpy( CurrentPage->ContentType, tmp, sizeof(CurrentPage->ContentType) );
         }
      }
      else if( !stricmp( "Content-Length", p ) )
      {
         char* tmp = strtok( 0, " " );
         if( tmp != NULL )
         {
            rc = sscanf( tmp, "%d", &page->ContentLength );
            if( rc != 1 )
            {
               printf( "could not get length\n" );
            }
            else
            {
               printf( "Content Length: %d\n", page->ContentLength );
            }
         }
      }
   } while( buffer[0] != 0 );

   if( path )
   {
      url = strtok( path, "?" );
      if( url == 0 )
      {
         url = "";
      }

      // Here is where we start parsing the arguments passed
      argc = 0;
      for( i=0; i<MAX_ARGV; i++ )
      {
         argv[i] = strtok( 0, "&" );
         if( argv[i] )
         {
            int      argLength;
            int      charValue;
            int      j;

            argLength = strlen(argv[i])+1;

            // Translate any escaped characters in the argument
            p = argv[i];
            result = p;
            for( j=0; j<argLength; j++ )
            {
               if( p[j] == '+' )
               {
                  *result++ = ' ';
               }
               else if( p[j] == '%' )
               {
                  char  s[3];
                  // This is an escaped character
                  snprintf( s, sizeof(s), "%c%c", p[j+1], p[j+2] );
                  sscanf( s, "%x", &charValue );
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

      //Page.Initialize( connection );
      if( PageHandler )
      {
         PageHandler( page, url, argc, argv );
      }

      connection->Flush();
      connection->Close();
   }
}

void HTTPD::Initialize( uint16_t port )
{
   int   i;

   PagePool.Initialize( MAX_ACTIVE_CONNECTIONS, "HTTPPage Pool" );
   for( i=0; i<MAX_ACTIVE_CONNECTIONS; i++ )
   {
      PagePool.Put( &PagePoolPages[ i ] );
   }

  ListenerConnection = ProtocolTCP::NewServer( port );
  Thread.Create( HTTPD::TaskEntry, "HTTPD", 1024*32, 100, 0 );
}

void HTTPD::ConnectionHandlerEntry( void* param )
{
   HTTPPage* page = (HTTPPage*)param;

   ProcessRequest( page );

   PagePool.Put( page );
}

void HTTPD::TaskEntry( void* param )
{
   ProtocolTCP::Connection* connection;
   HTTPPage* page;

   while( 1 )
   {
      connection = ListenerConnection->Listen();

      // Spawn off a thread to handle this connection
      page = (HTTPPage*)PagePool.Get();
      if( page )
      {
         page->Initialize( connection );
         page->Thread.Create( ConnectionHandlerEntry, "Page", 1024, 100, page );
      }
      else
      {
         printf( "Error: Out of pages\n" );
      }
   }
}

void HTTPD::SetDebug( bool enable )
{
   DebugFlag = enable;
}

bool HTTPD::Authorized( const char* username, const char* password, const char* url )
{
   return true;
}

