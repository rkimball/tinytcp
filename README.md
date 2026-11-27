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
`build/test/unit-test` runs the unit tests. For coverage, configure a Debug build with `-DTINYTCP_COVERAGE=ON` and run `make coverage` (it needs `gcovr`, which the dev container has). It writes `coverage/index.html` in the build directory and fails if line coverage is below the minimums in `CMakeLists.txt`. CI runs the same thing.

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
## Size
tinytcp is meant for small devices. All memory is statically allocated, there is no heap, and the
stack is built without exceptions, RTTI or `iostream`, so it needs next to nothing from the C++
runtime.

<!-- size-table:begin -->
| bytes | x86-64 | RISC-V (rv64) | ARM (Thumb-2) |
|---|---:|---:|---:|
| Stack code (text + data) | 21,326 | 20,776 | 11,508 |
| Stack RAM (`sizeof(DefaultStack)`) | 28,664 | 28,664 | 26,776 |
| OS layer code (text + data) | 6,436 | 6,063 | 3,619 |
| OS layer RAM (data + bss) | 1,462 | 1,462 | 874 |
| C++ runtime (libstdc++) linked in | 66 | 310 | 173 |
| Minimal program, static link | 885,633 | 549,411 | 388,064 |
| Empty program, same link | 711,349 | 490,387 | 341,664 |
| Net cost of the stack | 174,284 | 59,024 | 46,400 |
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
