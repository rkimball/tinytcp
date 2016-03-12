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
Linux build has been tested on Ubuntu 14.04. tinytcp does not support in-tree building.
```
mkdir myproject
cd myproject
git clone https://github.com/rkimball/tinytcp.git
mkdir build
cd build
cmake -G "Unix Makefiles" ../tinytcp
make
```

##Usage
The protocol stack has three main functions required

1. NetworkInterface.RxData - expects a [Layer 2 Ethernet frame](https://en.wikipedia.org/wiki/Ethernet_frame) as input
2. NetworkInterface.TxData - outputs a [Layer 2 Ethernet frame](https://en.wikipedia.org/wiki/Ethernet_frame)
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
