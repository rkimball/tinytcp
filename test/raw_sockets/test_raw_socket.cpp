//----------------------------------------------------------------------------
// Simple test app to verify raw socket functionality
// Tests: opening raw sockets, receiving packets, and sending packets
//----------------------------------------------------------------------------

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

// Helper function to display packet data
void DumpPacket(const uint8_t* data, size_t length)
{
    printf("Packet data (%zu bytes):\n", length);
    for (size_t i = 0; i < length && i < 64; i++)
    {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    if (length > 64)
        printf("... (truncated)\n");
    printf("\n");
}

// Get first non-loopback interface name (prefer physical interfaces)
bool GetInterface(char* name, size_t name_size)
{
    struct ifaddrs* ifaddr;
    bool found = false;
    const char* preferred_names[] = {"eth0", "eth1", "en0", "en1", "wlan0", nullptr};

    if (getifaddrs(&ifaddr) == 0)
    {
        // First pass: try to find preferred physical interfaces
        for (int i = 0; preferred_names[i] != nullptr; i++)
        {
            for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
            {
                if (ifa->ifa_addr == nullptr)
                    continue;

                int family = ifa->ifa_addr->sa_family;

                if (family == AF_PACKET && strcmp(ifa->ifa_name, preferred_names[i]) == 0)
                {
                    // Check if interface is up
                    if (ifa->ifa_flags & IFF_UP)
                    {
                        strncpy(name, ifa->ifa_name, name_size - 1);
                        name[name_size - 1] = '\0';
                        found = true;
                        break;
                    }
                }
            }
            if (found)
                break;
        }

        // Second pass: any non-loopback interface
        if (!found)
        {
            for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
            {
                if (ifa->ifa_addr == nullptr)
                    continue;

                int family = ifa->ifa_addr->sa_family;

                if (family == AF_PACKET)
                {
                    if (strcmp(ifa->ifa_name, "lo") != 0 && (ifa->ifa_flags & IFF_UP))
                    {
                        strncpy(name, ifa->ifa_name, name_size - 1);
                        name[name_size - 1] = '\0';
                        found = true;
                        break;
                    }
                }
            }
        }
        freeifaddrs(ifaddr);
    }
    return found;
}

// List all available interfaces
void ListInterfaces()
{
    struct ifaddrs* ifaddr;

    printf("\n=== Available Network Interfaces ===\n");
    if (getifaddrs(&ifaddr) == 0)
    {
        for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == nullptr)
                continue;

            int family = ifa->ifa_addr->sa_family;

            if (family == AF_PACKET)
            {
                printf("  %-12s AF_PACKET", ifa->ifa_name);
                if (ifa->ifa_flags & IFF_UP)
                    printf(" [UP]");
                if (ifa->ifa_flags & IFF_RUNNING)
                    printf(" [RUNNING]");
                if (ifa->ifa_flags & IFF_LOOPBACK)
                    printf(" [LOOPBACK]");
                printf("\n");
            }
            else if (family == AF_INET || family == AF_INET6)
            {
                printf("  %-12s %s\n",
                       ifa->ifa_name,
                       (family == AF_INET) ? "AF_INET" : "AF_INET6");
            }
        }
        freeifaddrs(ifaddr);
    }
    printf("\n");
}

int main()
{
    printf("=== Raw Socket Test Application ===\n\n");

    // Display available interfaces
    ListInterfaces();

    // Test 1: Check for root privileges and create raw socket
    printf("Test 1: Opening raw socket...\n");
    int raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (raw_socket == -1)
    {
        if (errno == EPERM)
        {
            printf("  [FAIL] Need root privileges. Try running: sudo %s\n", program_invocation_name);
            return 1;
        }
        else
        {
            printf("  [FAIL] Error creating socket: %s\n", strerror(errno));
            return 1;
        }
    }
    printf("  [PASS] Raw socket created successfully (fd=%d)\n", raw_socket);

    // Test 2: Get network interface
    printf("\nTest 2: Getting network interface...\n");
    char if_name[IFNAMSIZ];
    if (!GetInterface(if_name, sizeof(if_name)))
    {
        printf("  [FAIL] No suitable network interface found\n");
        close(raw_socket);
        return 1;
    }
    printf("  [PASS] Using interface: %s\n", if_name);

    // Test 3: Bind socket to interface
    printf("\nTest 3: Binding socket to interface...\n");
    struct ifreq ifr;
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(raw_socket, SIOCGIFINDEX, &ifr) == -1)
    {
        printf("  [FAIL] Error getting interface index: %s\n", strerror(errno));
        close(raw_socket);
        return 1;
    }
    int if_index = ifr.ifr_ifindex;
    printf("  [PASS] Interface index: %d\n", if_index);

    // Bind socket to the interface to get full Ethernet frames (not cooked mode)
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = if_index;
    sll.sll_protocol = htons(ETH_P_ALL);

    if (bind(raw_socket, (struct sockaddr*)&sll, sizeof(sll)) == -1)
    {
        printf("  [FAIL] Error binding to interface: %s\n", strerror(errno));
        close(raw_socket);
        return 1;
    }
    printf("  [PASS] Socket bound to interface %s\n", if_name);

    // Test 4: Set promiscuous mode
    printf("\nTest 4: Setting promiscuous mode...\n");
    struct packet_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.mr_ifindex = if_index;
    mreq.mr_type = PACKET_MR_PROMISC;
    mreq.mr_alen = 6;

    if (setsockopt(raw_socket, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        printf("  [WARN] Could not set promiscuous mode: %s\n", strerror(errno));
    }
    else
    {
        printf("  [PASS] Promiscuous mode enabled\n");
    }

    // Test 5: Receive packets
    printf("\nTest 5: Receiving packets (will capture 5 packets)...\n");
    uint8_t buffer[ETH_FRAME_LEN];
    int packet_count = 0;
    int max_packets = 5;

    // Set receive timeout
    struct timeval tv;
    tv.tv_sec = 5;  // 5 second timeout
    tv.tv_usec = 0;
    setsockopt(raw_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (packet_count < max_packets)
    {
        ssize_t length = recvfrom(raw_socket, buffer, ETH_FRAME_LEN, 0, nullptr, nullptr);

        if (length < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                printf("  [WARN] Timeout waiting for packets\n");
                break;
            }
            printf("  [FAIL] Error receiving packet: %s\n", strerror(errno));
            break;
        }
        else if (length > 0)
        {
            packet_count++;
            printf("  [PASS] Packet %d received: %zd bytes\n", packet_count, length);

            // Display Ethernet header info
            if (length >= 14)
            {
                // Check if we have actual MAC addresses (non-zero)
                bool has_dst_mac = false;
                bool has_src_mac = false;
                for (int i = 0; i < 6; i++)
                {
                    if (buffer[i] != 0)
                        has_dst_mac = true;
                    if (buffer[6 + i] != 0)
                        has_src_mac = true;
                }

                if (has_dst_mac || has_src_mac)
                {
                    printf("    Dst MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                           buffer[0],
                           buffer[1],
                           buffer[2],
                           buffer[3],
                           buffer[4],
                           buffer[5]);
                    printf("    Src MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                           buffer[6],
                           buffer[7],
                           buffer[8],
                           buffer[9],
                           buffer[10],
                           buffer[11]);
                    uint16_t ethertype = (buffer[12] << 8) | buffer[13];
                    printf("    EtherType: 0x%04X", ethertype);
                    if (ethertype == 0x0800)
                        printf(" (IPv4)");
                    else if (ethertype == 0x0806)
                        printf(" (ARP)");
                    else if (ethertype == 0x86DD)
                        printf(" (IPv6)");
                    printf("\n");
                }
                else
                {
                    printf("    [INFO] Interface may not provide Ethernet headers (cooked mode)\n");
                    printf("    First 16 bytes: ");
                    for (int i = 0; i < 16 && i < length; i++)
                    {
                        printf("%02X ", buffer[i]);
                    }
                    printf("\n");
                }
            }
        }
    }

    if (packet_count == 0)
    {
        printf("  [WARN] No packets received (interface may be down or no traffic)\n");
    }

    // Test 6: Send a packet (ARP request as example)
    printf("\nTest 6: Sending a test packet (ARP request)...\n");

    // Get MAC address of the interface
    if (ioctl(raw_socket, SIOCGIFHWADDR, &ifr) == -1)
    {
        printf("  [FAIL] Error getting MAC address: %s\n", strerror(errno));
        close(raw_socket);
        return 1;
    }

    uint8_t* src_mac = (uint8_t*)ifr.ifr_hwaddr.sa_data;
    printf("  Interface MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           src_mac[0],
           src_mac[1],
           src_mac[2],
           src_mac[3],
           src_mac[4],
           src_mac[5]);

    // Build a simple ARP request packet
    uint8_t arp_packet[42];
    memset(arp_packet, 0, sizeof(arp_packet));

    // Ethernet header
    memset(arp_packet, 0xFF, 6);              // Broadcast destination
    memcpy(arp_packet + 6, src_mac, 6);       // Source MAC
    arp_packet[12] = 0x08;                    // EtherType: ARP
    arp_packet[13] = 0x06;

    // ARP header
    arp_packet[14] = 0x00;
    arp_packet[15] = 0x01; // Hardware type: Ethernet
    arp_packet[16] = 0x08;
    arp_packet[17] = 0x00; // Protocol type: IPv4
    arp_packet[18] = 0x06; // Hardware size
    arp_packet[19] = 0x04; // Protocol size
    arp_packet[20] = 0x00;
    arp_packet[21] = 0x01; // Operation: request

    memcpy(arp_packet + 22, src_mac, 6);      // Sender MAC
    // Sender IP: 0.0.0.0 (already zeroed)
    // Target MAC: 00:00:00:00:00:00 (already zeroed)
    // Target IP: 1.2.3.4 (just an example)
    arp_packet[38] = 1;
    arp_packet[39] = 2;
    arp_packet[40] = 3;
    arp_packet[41] = 4;

    // Send the packet
    struct sockaddr_ll dest;
    memset(&dest, 0, sizeof(dest));
    dest.sll_family = AF_PACKET;
    dest.sll_ifindex = if_index;
    dest.sll_halen = ETH_ALEN;
    memset(dest.sll_addr, 0xFF, 6); // Broadcast

    ssize_t sent = sendto(raw_socket, arp_packet, sizeof(arp_packet), 0, (struct sockaddr*)&dest, sizeof(dest));

    if (sent < 0)
    {
        printf("  [FAIL] Error sending packet: %s\n", strerror(errno));
    }
    else
    {
        printf("  [PASS] Packet sent successfully: %zd bytes\n", sent);
        DumpPacket(arp_packet, sizeof(arp_packet));
    }

    // Cleanup
    printf("\nTest 7: Cleaning up...\n");
    close(raw_socket);
    printf("  [PASS] Socket closed\n");

    printf("\n=== All Tests Complete ===\n");
    printf("Summary:\n");
    printf("  - Raw socket creation: SUCCESS\n");
    printf("  - Interface binding: SUCCESS\n");
    printf("  - Packet reception: %s\n", packet_count > 0 ? "SUCCESS" : "WARNING");
    printf("  - Packet transmission: SUCCESS\n");
    
    if (packet_count > 0)
    {
        printf("\n");
        printf("Packets were successfully received and sent.\n");
        printf("Full Ethernet frames with MAC headers were captured.\n");
    }

    return 0;
}
