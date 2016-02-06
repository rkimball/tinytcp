# tinytcp
tinytcp is designed primarily for use in an embedded environment. It is designed for simplicity and deterministic memory usage.
All memory used is preallocated in static arrays.

##Building
Building uses [CMake](https://cmake.org/) and has been tested with cmake version 3.4.0-rc3.
###Windows
Windows build has been tested on Windows 10 using the community version of [Visual Studio 2015](https://www.visualstudio.com/en-us/visual-studio-homepage-vs.aspx).
```
cmake -G "Visual Studio 14 2015" ..\tinytcp
```
###Linux
Linux build has been tested on Ubuntu 14.04
```
cmake -G "Unix Makefiles" ..\tinytcp
```

##Usage
The protocol stack has three main functions required

1. NetworkInterface.RxData
2. NetworkInterface.TxData
3. ProtocolTCP::Tick()

Doing something useful:
```c_cpp
   ListenerConnection = ProtocolTCP::NewServer( port );
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
         printf( "Error: Out of pages\\n" );
      }
   }
```
