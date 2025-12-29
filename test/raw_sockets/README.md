# Raw Socket Test Application

This is a simple test application to verify raw socket functionality on Linux.

## What It Tests

1. **Raw Socket Creation** - Verifies that raw sockets can be opened (requires root privileges)
2. **Network Interface Discovery** - Lists and selects a network interface
3. **Interface Binding** - Binds the socket to a specific network interface
4. **Promiscuous Mode** - Sets the interface to promiscuous mode to capture all traffic
5. **Packet Reception** - Receives and analyzes incoming Ethernet frames
6. **Packet Transmission** - Sends a test ARP request packet
7. **Resource Cleanup** - Properly closes the socket

## Building

From the project root directory:

```bash
cd build
cmake ..
make test_raw_socket
```

## Running

### In the Dev Container (Recommended)

When using the VS Code dev container, the test binary automatically receives the
`CAP_NET_RAW` capability during the build process. This means you can run tests
**without sudo**:

```bash
cd build/test/raw_sockets
./test_raw_socket
```

The dev container is configured with:
- `--network=host` - Shares the host's network namespace
- `--cap-add=NET_RAW` - Grants the container CAP_NET_RAW capability
- `--cap-add=NET_ADMIN` - Grants the container CAP_NET_ADMIN capability

The build system uses `setcap` to apply these capabilities to the test executable.

### Outside the Dev Container

If running outside the dev container, you have two options:

1. **Set capabilities manually** (preferred):
   ```bash
   sudo setcap cap_net_raw,cap_net_admin+eip ./test_raw_socket
   ./test_raw_socket
   ```

2. **Run with sudo**:
   ```bash
   sudo ./test_raw_socket
   ```

## Expected Output

The application will:
- List all available network interfaces
- Create a raw socket
- Bind to the first non-loopback interface
- Capture 5 packets and display their Ethernet headers
- Send a test ARP request packet
- Display a summary of test results

## Requirements

- Linux operating system
- Root/sudo privileges (raw sockets require CAP_NET_RAW capability)
- Active network interface (other than loopback)

## Exit Codes

- `0` - All tests passed successfully
- `1` - Test failure (details printed to stdout)

## Notes

- The test uses a 5-second timeout for packet reception
- If no packets are received, the interface may be down or have no traffic
- The ARP request sent is a harmless broadcast packet requesting IP 1.2.3.4
- MAC addresses are displayed in the standard format (XX:XX:XX:XX:XX:XX)
