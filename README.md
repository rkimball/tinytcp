# tinytcp
tinytcp is designed primarily for use in an embedded environment. It is designed for simplicity and deterministic memory usage.
All memory used is preallocated in static arrays.

## Building
Building uses [CMake](https://cmake.org/) and has been tested with cmake version 3.4.0-rc3.
### Windows
Windows build has been tested on Windows 10 using the community version of [Visual Studio 2015](https://www.visualstudio.com/en-us/visual-studio-homepage-vs.aspx).
```
mkdir myproject
cd myproject
git clone https://github.com/rkimball/tinytcp.git
mkdir build
cd build
cmake -G "Visual Studio 14 2015" ..\tinytcp
```
This generates tinytcp.sln solution file.
* The Windows build uses [WinPcap](http://www.winpcap.org/) that you will need to download and install before running.
* To view the tinytcp testApp web page you will need to point a browser at the IPv4 address that testApp gets from DHCP. This is printed out when the app is run. You will also need to use a browser on a different computer that the one running testApp. When run, testApp will produce output like this
```
sending discover
discover sent
DHCP Send type 3
DHCP got address 192.168.1.23
```
so you will point your browser at http://192.168.1.23 in this example
### Linux
Linux build has been tested on Ubuntu 14.04 and 16.04. It works fine in a Windows Hyper-v VM
install of Ubuntu. tinytcp does not support in-tree building.

```
mkdir myproject
cd myproject
git clone https://github.com/rkimball/tinytcp.git
mkdir build
cd build
cmake ../tinytcp
make
```
* Because the tinytcp test app uses a promiscuous socket in order to read/write Layer 2 Ethernet frames, the test app must be run
with elevated privileges.
```
sudo testApp/testApp
```
* To view the tinytcp testApp web page you will need to point a browser at the IPv4 address that testApp gets from DHCP. This is printed out when the app is run. You will also need to use a browser on a different computer that the one running testApp. When run, testApp will produce output like this:
```
sending discover
discover sent
DHCP Send type 3
DHCP got address 192.168.1.23
```
so you will point your browser at http://192.168.1.23 in this example
* To run on a windows Hyper-V ubuntu installation you need to enable MAC address spoofing on the
VM's network adapter. To do this go to the VM's settings page, expand the *Network Adapter* node in
the left panel, and select *Advanced Features*. Now, in the panel on the right check the
*Enable MAC address spoofing* checkbox and select *Apply* to apply the setting.
## Usage
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
## TCP Library Size
All of the memory used is statically allocated and so an buffer such as transmit or receive will
show up in the bss section. The transmit and receive buffers are configurable and current set to 20 each of size 512 bytes.
These buffers are defined in ProtocolMACEthernet, which explains it's large bss.
```
   text	   data	    bss	    dec	    hex	filename
   1249	      8	     48	   1305	    519	Address.cpp.o (ex build/tcpStack/libtcpStack.a)
      0	      0	      0	      0	      0	DataBuffer.cpp.o (ex build/tcpStack/libtcpStack.a)
    312	      0	      0	    312	    138	FCS.cpp.o (ex build/tcpStack/libtcpStack.a)
    164	      0	      0	    164	     a4	NetworkInterface.cpp.o (ex build/tcpStack/libtcpStack.a)
   3524	      8	    600	   4132	   1024	ProtocolARP.cpp.o (ex build/tcpStack/libtcpStack.a)
   5018	      4	      0	   5022	   139e	ProtocolDHCP.cpp.o (ex build/tcpStack/libtcpStack.a)
    360	      0	      0	    360	    168	ProtocolICMP.cpp.o (ex build/tcpStack/libtcpStack.a)
   1665	      8	    320	   1993	    7c9	ProtocolIP.cpp.o (ex build/tcpStack/libtcpStack.a)
   2176	      8	  22232	  24416	   5f60	ProtocolMACEthernet.cpp.o (ex build/tcpStack/libtcpStack.a)
   8873	      8	   3744	  12625	   3151	ProtocolTCP.cpp.o (ex build/tcpStack/libtcpStack.a)
    717	      0	      0	    717	    2cd	ProtocolUDP.cpp.o (ex build/tcpStack/libtcpStack.a)
   2731	      0	     84	   2815	    aff	Utility.cpp.o (ex build/tcpStack/libtcpStack.a)
```
