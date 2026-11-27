The test app runs a small web server on top of tinytcp. It uses a Linux raw (`AF_PACKET`) socket to send and receive Ethernet frames, so it needs `CAP_NET_RAW` (for example, run it with `sudo`).

Command line options:
	-devices	List the network interfaces found.

The interface used is the first non-loopback interface, or the one named by the `TINYTCP_IFACE` environment variable.

`vnet.sh` runs the app on a virtual network (veth pair plus a DHCP server) for testing inside a container. See the top-level README.
