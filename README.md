![main](https://github.com/rkimball/tinytcp/actions/workflows/cmake.yml/badge.svg)

# tinytcp
tinytcp is designed primarily for use in an embedded environment. It is designed for simplicity and deterministic memory usage.
All memory used is preallocated in static arrays.

## Building
Building uses [CMake](https://cmake.org/).
### Linux
tinytcp builds and runs on Linux (tested on Ubuntu 22.04). Windows is no longer supported.
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
sudo test_app/test_app
```
* To view the tinytcp test_app web page you will need to point a browser at the IPv4 address that test_app gets from DHCP. This is printed out when the app is run. You will also need to use a browser on a different computer that the one running test_app. When run, test_app will produce output like this:
```
sending discover
discover sent
DHCP Send type 3
DHCP got address 192.168.1.23
```
so you will point your browser at http://192.168.1.23 in this example

## Dev Container
Use **Dev Containers: Reopen in Container** in VS Code and pick a configuration:
* **NoChatSupport** - build environment only.
* **ClaudeSupport** - adds Claude Code. It mounts your host `~/.claude` so history is shared with the container. Set `export CLAUDE_CONFIG_DIR="$HOME/.claude"` on the host so the host and container share one `.claude.json`.

### Testing in the container
`test_app/vnet.sh test` builds a virtual network (veth pair plus a dnsmasq DHCP server), runs `test_app` on it, fetches its web page with curl and cleans up. `test_app/vnet.sh run` keeps it running and forwards the web server to port 8080 so a browser on the host can reach it. Build first (`cmake` + `make` in `build/`).

### Unit tests and coverage
`build/test/unit-test` runs the unit tests. For coverage, configure a Debug build with `-DTINYTCP_COVERAGE=ON` and run `make coverage` (it needs `gcovr`, which the dev container has). It writes `coverage/index.html` in the build directory and fails if line coverage is below the minimums in `CMakeLists.txt`. CI runs the same thing, and publishes the report for `main` at https://rkimball.github.io/tinytcp/.

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
## RFC conformance
tinytcp is checked against the RFCs that define each protocol it implements. A box is ticked only
when the unit tests written for that RFC all pass, so ☑ means "tested and conforming", not "believed
to work". Tests are named after the RFC they check and live in `test/tinytcp/`. Work goes from the
bottom layer up. RFCs marked *(optional)* are not needed to finish their layer.

☐ not tested yet, ☑ tests pass

Where an RFC has been replaced the current one is used: TCP is checked against RFC 9293, which
obsoletes RFC 793 and folds in RFCs 879, 6528 and 6691. RFC 1122 (host requirements) applies to
several layers, so each layer lists the part of it that applies.

### Link: Ethernet
| | RFC | Title | What the tests check | Tests |
|:-:|---|---|---|---|
| ☑ | [RFC 894](https://www.rfc-editor.org/rfc/rfc894) | A Standard for the Transmission of IP Datagrams over Ethernet Networks | Frame layout, EtherType values for IP (0x0800) and ARP (0x0806), zero padding to 46 octets of data and ignoring padding on receive, the broadcast address, frames shorter than a header dropped. Receive buffers hold 592 octets (a 576 octet datagram), so larger frames are dropped. The RFC allows that: a host that cannot take full-length frames should discourage them, and TCP without an MSS option keeps peers to 536 octet segments | [test_ethernet.cpp](test/tinytcp/test_ethernet.cpp) |

### Link to network: ARP
| | RFC | Title | What the tests check | Tests |
|:-:|---|---|---|---|
| ☑ | [RFC 826](https://www.rfc-editor.org/rfc/rfc826) | An Ethernet Address Resolution Protocol | Packet format, reply to a request for our address, the packet reception rules in the RFC's order (hardware and protocol type and address length checks, replacing a changed address, learning the sender, ignoring packets for other hosts), asking for an address, a full table. Does not cover when entries expire, which RFC 826 leaves open (see RFC 1122 sec. 2.3.2) | [test_arp.cpp](test/tinytcp/test_arp.cpp) |
| ☐ | [RFC 1122](https://www.rfc-editor.org/rfc/rfc1122) sec. 2.3.2 | Requirements for Internet Hosts, ARP | Cache timeout and limiting how often requests are sent *(optional)*. Not met: entries age by the number of ARP events, not by time, and requests are not rate limited | |
| ☐ | [RFC 5227](https://www.rfc-editor.org/rfc/rfc5227) | IPv4 Address Conflict Detection | Probing before using an address, defending it *(optional)*. Not implemented | |

### Internet: IPv4 and ICMP
| | RFC | Title | What the tests check | Tests |
|:-:|---|---|---|---|
| ☐ | [RFC 791](https://www.rfc-editor.org/rfc/rfc791) | Internet Protocol | Header fields and their validation, options skipped using the header length, fragment fields, accepting 576 byte datagrams, protocol demultiplexing, header of the datagrams we send | |
| ☑ | [RFC 1071](https://www.rfc-editor.org/rfc/rfc1071) | Computing the Internet Checksum | One's complement sum, odd length data, carry folding. Used by IP, ICMP, UDP and TCP | [test_checksum.cpp](test/tinytcp/test_checksum.cpp) |
| ☐ | [RFC 1122](https://www.rfc-editor.org/rfc/rfc1122) sec. 3.2 | Requirements for Internet Hosts, IP | Silently discard a wrong version or a bad header checksum, accept only our own and broadcast addresses, TTL of datagrams we send | |
| ☐ | [RFC 919](https://www.rfc-editor.org/rfc/rfc919), [RFC 922](https://www.rfc-editor.org/rfc/rfc922) | Broadcasting Internet Datagrams | Limited broadcast (255.255.255.255) and subnet broadcast, which DHCP needs | |
| ☐ | [RFC 950](https://www.rfc-editor.org/rfc/rfc950) | Internet Standard Subnetting Procedure | Using the subnet mask to decide between the local network and the gateway | |
| ☐ | [RFC 6864](https://www.rfc-editor.org/rfc/rfc6864) | Updated Specification of the IPv4 ID Field | ID field of the datagrams we send *(optional)* | |
| ☐ | [RFC 2474](https://www.rfc-editor.org/rfc/rfc2474), [RFC 3168](https://www.rfc-editor.org/rfc/rfc3168) | DS Field and ECN | DS and ECN bits are ignored on receive *(optional)* | |
| ☐ | [RFC 792](https://www.rfc-editor.org/rfc/rfc792) | Internet Control Message Protocol | Echo request and reply (identifier, sequence and data returned unchanged, checksum), destination unreachable format, other types ignored | |
| ☐ | [RFC 1122](https://www.rfc-editor.org/rfc/rfc1122) sec. 3.2.2 | Requirements for Internet Hosts, ICMP | Must answer echo requests, errors that must and must not be sent | |

### Transport: UDP
| | RFC | Title | What the tests check | Tests |
|:-:|---|---|---|---|
| ☐ | [RFC 768](https://www.rfc-editor.org/rfc/rfc768) | User Datagram Protocol | Header format, checksum with the pseudo header, a checksum of zero means none, length validation, delivery to the right port | |
| ☐ | [RFC 1122](https://www.rfc-editor.org/rfc/rfc1122) sec. 4.1 | Requirements for Internet Hosts, UDP | Checksum rules, ICMP port unreachable when nothing listens | |
| ☐ | [RFC 9868](https://www.rfc-editor.org/rfc/rfc9868) | Transport Options for UDP | Options sit after the UDP length inside the IP datagram, so only the UDP length is used for the data *(optional)* | |

### Transport: TCP
| | RFC | Title | What the tests check | Tests |
|:-:|---|---|---|---|
| ☐ | [RFC 9293](https://www.rfc-editor.org/rfc/rfc9293) | Transmission Control Protocol (TCP) | Header and checksum, every state and transition, three way handshake and simultaneous open, reset generation and processing, closing, TIME-WAIT, sequence numbers and wraparound, windows, retransmission, MSS option, initial sequence numbers | |
| ☐ | [RFC 1122](https://www.rfc-editor.org/rfc/rfc1122) sec. 4.2 | Requirements for Internet Hosts, TCP | Parts 9293 leaves to it: delayed ACK, silly window avoidance, window probes | |
| ☐ | [RFC 6298](https://www.rfc-editor.org/rfc/rfc6298) | Computing TCP's Retransmission Timer | Smoothed RTT and variance, the timeout, backoff, minimum timeout | |
| ☐ | [RFC 5681](https://www.rfc-editor.org/rfc/rfc5681) | TCP Congestion Control | Slow start, congestion avoidance, fast retransmit *(to be decided)* | |
| ☐ | [RFC 5961](https://www.rfc-editor.org/rfc/rfc5961) | Improving TCP's Robustness to Blind In-Window Attacks | Checks on RST, SYN and ACK segments *(optional)* | |
| ☐ | [RFC 7323](https://www.rfc-editor.org/rfc/rfc7323) | TCP Extensions for High Performance | Window scale and timestamps are ignored cleanly if offered *(optional)* | |
| ☐ | [RFC 2018](https://www.rfc-editor.org/rfc/rfc2018) | TCP Selective Acknowledgment Options | SACK is ignored cleanly if offered *(optional)* | |

### Configuration: DHCP
| | RFC | Title | What the tests check | Tests |
|:-:|---|---|---|---|
| ☐ | [RFC 2131](https://www.rfc-editor.org/rfc/rfc2131) | Dynamic Host Configuration Protocol | Fixed header and magic cookie, DISCOVER and REQUEST we send, handling OFFER, ACK and NAK, transaction ID matching, lease, renewing and rebinding | |
| ☐ | [RFC 2132](https://www.rfc-editor.org/rfc/rfc2132) | DHCP Options and BOOTP Vendor Extensions | The options we send and read: subnet mask, router, DNS, broadcast address, lease time, message type, renewal and rebinding times, end and pad | |
| ☐ | [RFC 951](https://www.rfc-editor.org/rfc/rfc951), [RFC 1542](https://www.rfc-editor.org/rfc/rfc1542) | Bootstrap Protocol and its clarifications | The BOOTP message layout DHCP is built on, ports 67 and 68, the broadcast flag | |
| ☐ | [RFC 3396](https://www.rfc-editor.org/rfc/rfc3396) | Encoding Long Options in DHCPv4 | Options split over several entries *(optional)* | |

### Application: HTTP (`src/web_server`)
| | RFC | Title | What the tests check | Tests |
|:-:|---|---|---|---|
| ☐ | [RFC 1945](https://www.rfc-editor.org/rfc/rfc1945) | Hypertext Transfer Protocol, HTTP/1.0 | What the server speaks today: request line, status line, headers, 200 and 404 | |
| ☐ | [RFC 9110](https://www.rfc-editor.org/rfc/rfc9110), [RFC 9112](https://www.rfc-editor.org/rfc/rfc9112) | HTTP Semantics and HTTP/1.1 | The current HTTP standards *(optional, later)* | |

## Size
tinytcp is meant for small devices. All memory is statically allocated, there is no heap, and the
stack is built without exceptions, RTTI or `iostream`, so it needs next to nothing from the C++
runtime.

<!-- size-table:begin -->
| bytes | x86-64 | RISC-V (rv64) | ARM (Thumb-2) |
|---|---:|---:|---:|
| Stack code (text + data) | 21,603 | 21,020 | 11,668 |
| Stack RAM (`sizeof(DefaultStack)`) | 32,024 | 32,024 | 30,136 |
| OS layer code (text + data) | 6,946 | 6,615 | 4,002 |
| OS layer RAM (data + bss) | 1,462 | 1,462 | 874 |
| C++ runtime (libstdc++) linked in | 66 | 310 | 173 |
| Minimal program, static link | 890,265 | 553,923 | 392,168 |
| Empty program, same link | 711,349 | 490,387 | 341,664 |
| Net cost of the stack | 178,916 | 63,536 | 50,504 |
<!-- size-table:end -->

* **Stack code** is `src/tinytcp` (text + data). **Stack RAM** is `sizeof(DefaultStack)`, which holds
  every buffer the stack uses. It is set in `src/tinytcp/Config.hpp`: 20 transmit and 20 receive
  buffers of 512 bytes, 5 TCP connections and so on.
* **OS layer** is `src/os`: threads, mutexes, events, queues and a clock. This is the part to port
  to another RTOS.
* **C++ runtime** is what is left of libstdc++ in a program that uses the whole stack. A host
  written in C has to run global constructors and provide `__cxa_atexit` and `operator delete`,
  or just link with `g++`.
* **Net cost** is a program that uses the stack minus an empty program, linked the same way. The
  two program rows are statically linked against the toolchain's glibc, which is much bigger than
  an embedded libc, so they are mostly libc. Read the net cost, not the totals.
* The stack itself calls only `printf` (log messages) and `sprintf` from the C library.

Everything is built with `-Os -fno-exceptions -fno-rtti -fno-threadsafe-statics`, with the host
`g++` for x86-64 and the GNU cross compilers for RISC-V (rv64gc) and 32-bit ARM (Thumb-2).

Run `make size` in the build directory (or `tools/size/measure.sh`) to print this table, and
`make size-readme` to update it here. `tools/size/measure.sh --details` also shows where the
linked bytes come from. The dev container has all three compilers.
