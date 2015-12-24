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

#ifndef HTTPD_H
#define HTTPD_H

#include "HTTPPage.h"

#define MAX_ACTIVE_CONNECTIONS 3
#define HTTPD_PATH_LENGTH_MAX    256

typedef void( *PageRequestHandler )(HTTPPage* page, const char* url, int argc, char** argv);

class HTTPD
{
   friend class HTTPPage;

public:
   HTTPD(){}
   static void RegisterPageHandler( PageRequestHandler );
   static void TaskEntry( void* param );

   static void Initialize( uint16_t port );
   static void SetDebug( bool );

   static bool Authorized( const char* username, const char* password, const char* url );
   static void ProcessRequest( HTTPPage* page );

private:
   HTTPD( HTTPD& );

   static void ConnectionHandlerEntry( void* );

   static bool          DebugFlag;
   static HTTPPage      PagePoolPages[ MAX_ACTIVE_CONNECTIONS ];
   static osQueue       PagePool;
   static PageRequestHandler PageHandler;
};

#endif
