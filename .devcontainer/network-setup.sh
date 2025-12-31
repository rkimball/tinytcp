#!/bin/bash
# Network setup script for tinytcp development
# This script is run when the devcontainer starts

echo "=== TinyTCP Network Setup ==="

# Display available network interfaces
echo "Available network interfaces:"
ls /sys/class/net/ | while read iface; do
    if [ -f "/sys/class/net/$iface/address" ]; then
        mac=$(cat /sys/class/net/$iface/address)
        echo "  $iface: $mac"
    fi
done

# Show preferred interface for tinytcp (eth0 preferred)
if [ -d /sys/class/net/eth0 ]; then
    echo ""
    echo "TinyTCP will use: eth0"
    cat /sys/class/net/eth0/address
fi

echo ""
echo "Network setup complete."
echo ""
echo "To start a local DHCP server for testing, run:"
echo "  sudo dnsmasq --no-daemon --interface=eth0 --bind-interfaces \\"
echo "    --dhcp-range=172.17.0.100,172.17.0.200,12h --log-dhcp"
echo ""
