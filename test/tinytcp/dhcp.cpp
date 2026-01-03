#include <gtest/gtest.h>
#include <cstring>
#include "data_buffer.hpp"
#include "utility.hpp"

using namespace tinytcp;

// ============================================================================
// DHCP Packet Structure Tests
// These tests verify the structure and parsing of DHCP packets
// ============================================================================

// DHCP Message Types
static const uint8_t DHCP_DISCOVER = 1;
static const uint8_t DHCP_OFFER = 2;
static const uint8_t DHCP_REQUEST = 3;
static const uint8_t DHCP_DECLINE = 4;
static const uint8_t DHCP_ACK = 5;
static const uint8_t DHCP_NAK = 6;
static const uint8_t DHCP_RELEASE = 7;

// DHCP Options
static const uint8_t DHCP_OPTION_SUBNET_MASK = 1;
static const uint8_t DHCP_OPTION_ROUTER = 3;
static const uint8_t DHCP_OPTION_DNS = 6;
static const uint8_t DHCP_OPTION_HOSTNAME = 12;
static const uint8_t DHCP_OPTION_BROADCAST = 28;
static const uint8_t DHCP_OPTION_REQUESTED_IP = 50;
static const uint8_t DHCP_OPTION_LEASE_TIME = 51;
static const uint8_t DHCP_OPTION_MESSAGE_TYPE = 53;
static const uint8_t DHCP_OPTION_SERVER_ID = 54;
static const uint8_t DHCP_OPTION_PARAM_REQUEST = 55;
static const uint8_t DHCP_OPTION_RENEW_TIME = 58;
static const uint8_t DHCP_OPTION_REBIND_TIME = 59;
static const uint8_t DHCP_OPTION_CLIENT_ID = 61;
static const uint8_t DHCP_OPTION_END = 255;

// DHCP Magic Cookie
static const uint32_t DHCP_MAGIC_COOKIE = 0x63825363;

// Helper function to create a basic DHCP packet structure
void CreateDHCPPacket(uint8_t* packet, size_t& offset, uint8_t op, uint32_t xid) {
    offset = 0;
    offset = Pack8(packet, offset, op);         // op: 1=request, 2=reply
    offset = Pack8(packet, offset, 1);          // htype: Ethernet
    offset = Pack8(packet, offset, 6);          // hlen: MAC address length
    offset = Pack8(packet, offset, 0);          // hops
    offset = Pack32(packet, offset, xid);       // xid: transaction ID
    offset = Pack16(packet, offset, 0);         // secs: seconds elapsed
    offset = Pack16(packet, offset, 0x8000);    // flags: broadcast
    offset = Pack32(packet, offset, 0);         // ciaddr: client IP
    offset = Pack32(packet, offset, 0);         // yiaddr: your IP
    offset = Pack32(packet, offset, 0);         // siaddr: server IP
    offset = Pack32(packet, offset, 0);         // giaddr: gateway IP
    
    // chaddr: client hardware address (16 bytes)
    for (int i = 0; i < 16; i++) {
        packet[offset++] = (i < 6) ? (0xAA + i) : 0;
    }
    
    // sname: server hostname (64 bytes)
    for (int i = 0; i < 64; i++) {
        packet[offset++] = 0;
    }
    
    // file: boot filename (128 bytes)
    for (int i = 0; i < 128; i++) {
        packet[offset++] = 0;
    }
    
    // Magic cookie
    offset = Pack32(packet, offset, DHCP_MAGIC_COOKIE);
}

// Helper to add DHCP option
void AddDHCPOption(uint8_t* packet, size_t& offset, uint8_t option, uint8_t length, const uint8_t* data) {
    packet[offset++] = option;
    packet[offset++] = length;
    for (int i = 0; i < length; i++) {
        packet[offset++] = data[i];
    }
}

// Helper to find DHCP option in packet
bool FindDHCPOption(const uint8_t* packet, size_t startOffset, size_t endOffset, 
                    uint8_t optionCode, size_t& optionOffset, uint8_t& optionLength) {
    size_t offset = startOffset;
    
    while (offset < endOffset) {
        uint8_t option = packet[offset++];
        
        if (option == DHCP_OPTION_END) {
            return false;
        }
        
        if (offset >= endOffset) {
            return false;
        }
        
        uint8_t length = packet[offset++];
        
        if (option == optionCode) {
            optionOffset = offset;
            optionLength = length;
            return true;
        }
        
        offset += length;
    }
    
    return false;
}

// ============================================================================
// DHCP Header Structure Tests
// ============================================================================

TEST(DHCPPacket, HeaderBasicStructure) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0x12345678);
    
    // Verify header fields
    EXPECT_EQ(Unpack8(packet, 0), 1);           // op: BOOTREQUEST
    EXPECT_EQ(Unpack8(packet, 1), 1);           // htype: Ethernet
    EXPECT_EQ(Unpack8(packet, 2), 6);           // hlen: 6 bytes
    EXPECT_EQ(Unpack8(packet, 3), 0);           // hops: 0
    EXPECT_EQ(Unpack32(packet, 4), 0x12345678); // xid
    EXPECT_EQ(Unpack16(packet, 8), 0);          // secs
    EXPECT_EQ(Unpack16(packet, 10), 0x8000);    // flags: broadcast bit
}

TEST(DHCPPacket, HeaderIPAddressFields) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 2, 0xABCDEF00);
    
    // Set IP addresses
    Pack32(packet, 12, 0xC0A80101);  // ciaddr: 192.168.1.1
    Pack32(packet, 16, 0xC0A80164);  // yiaddr: 192.168.1.100
    Pack32(packet, 20, 0xC0A80101);  // siaddr: 192.168.1.1
    Pack32(packet, 24, 0xC0A80101);  // giaddr: 192.168.1.1
    
    EXPECT_EQ(Unpack32(packet, 12), 0xC0A80101);  // ciaddr
    EXPECT_EQ(Unpack32(packet, 16), 0xC0A80164);  // yiaddr
    EXPECT_EQ(Unpack32(packet, 20), 0xC0A80101);  // siaddr
    EXPECT_EQ(Unpack32(packet, 24), 0xC0A80101);  // giaddr
}

TEST(DHCPPacket, HeaderMACAddress) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0x11223344);
    
    // Verify MAC address in chaddr field (offset 28)
    EXPECT_EQ(packet[28], 0xAA);
    EXPECT_EQ(packet[29], 0xAB);
    EXPECT_EQ(packet[30], 0xAC);
    EXPECT_EQ(packet[31], 0xAD);
    EXPECT_EQ(packet[32], 0xAE);
    EXPECT_EQ(packet[33], 0xAF);
    
    // Remaining chaddr bytes should be zero
    for (int i = 6; i < 16; i++) {
        EXPECT_EQ(packet[28 + i], 0);
    }
}

TEST(DHCPPacket, HeaderMagicCookie) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0x99887766);
    
    // Magic cookie is at offset 236
    uint32_t magic = Unpack32(packet, 236);
    EXPECT_EQ(magic, DHCP_MAGIC_COOKIE);
    EXPECT_EQ(magic, 0x63825363);
}

TEST(DHCPPacket, HeaderFixedSize) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0);
    
    // DHCP header should be exactly 240 bytes (236 + 4 for magic cookie)
    EXPECT_EQ(offset, 240);
}

// ============================================================================
// DHCP Options Tests
// ============================================================================

TEST(DHCPOptions, MessageTypeOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0x12345678);
    
    // Add DHCP Message Type option
    uint8_t messageType = DHCP_DISCOVER;
    AddDHCPOption(packet, offset, DHCP_OPTION_MESSAGE_TYPE, 1, &messageType);
    packet[offset++] = DHCP_OPTION_END;
    
    // Find and verify the option
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_MESSAGE_TYPE, 
                                 optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 1);
    EXPECT_EQ(packet[optionOffset], DHCP_DISCOVER);
}

TEST(DHCPOptions, ServerIdentifierOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 2, 0xAABBCCDD);
    
    // Add Server Identifier option
    uint8_t serverIP[] = {192, 168, 1, 1};
    AddDHCPOption(packet, offset, DHCP_OPTION_SERVER_ID, 4, serverIP);
    packet[offset++] = DHCP_OPTION_END;
    
    // Find and verify
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_SERVER_ID,
                                 optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 4);
    EXPECT_EQ(packet[optionOffset], 192);
    EXPECT_EQ(packet[optionOffset + 1], 168);
    EXPECT_EQ(packet[optionOffset + 2], 1);
    EXPECT_EQ(packet[optionOffset + 3], 1);
}

TEST(DHCPOptions, RequestedIPAddressOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0x55667788);
    
    // Add Requested IP Address option
    uint8_t requestedIP[] = {192, 168, 1, 100};
    AddDHCPOption(packet, offset, DHCP_OPTION_REQUESTED_IP, 4, requestedIP);
    packet[offset++] = DHCP_OPTION_END;
    
    // Find and verify
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_REQUESTED_IP,
                                 optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 4);
    uint32_t ip = Unpack32(packet, optionOffset);
    EXPECT_EQ(ip, 0xC0A80164);  // 192.168.1.100
}

TEST(DHCPOptions, SubnetMaskOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 2, 0x11111111);
    
    // Add Subnet Mask option
    uint8_t subnetMask[] = {255, 255, 255, 0};
    AddDHCPOption(packet, offset, DHCP_OPTION_SUBNET_MASK, 4, subnetMask);
    packet[offset++] = DHCP_OPTION_END;
    
    // Find and verify
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_SUBNET_MASK,
                                 optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 4);
    EXPECT_EQ(Unpack32(packet, optionOffset), 0xFFFFFF00);
}

TEST(DHCPOptions, RouterOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 2, 0x22222222);
    
    // Add Router option
    uint8_t router[] = {192, 168, 1, 1};
    AddDHCPOption(packet, offset, DHCP_OPTION_ROUTER, 4, router);
    packet[offset++] = DHCP_OPTION_END;
    
    // Find and verify
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_ROUTER,
                                 optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 4);
    EXPECT_EQ(Unpack32(packet, optionOffset), 0xC0A80101);
}

TEST(DHCPOptions, DNSOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 2, 0x33333333);
    
    // Add DNS option
    uint8_t dns[] = {8, 8, 8, 8};
    AddDHCPOption(packet, offset, DHCP_OPTION_DNS, 4, dns);
    packet[offset++] = DHCP_OPTION_END;
    
    // Find and verify
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_DNS,
                                 optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 4);
    EXPECT_EQ(Unpack32(packet, optionOffset), 0x08080808);
}

TEST(DHCPOptions, LeaseTimeOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 2, 0x44444444);
    
    // Add Lease Time option (86400 seconds = 1 day)
    uint8_t leaseTime[4];
    Pack32(leaseTime, 0, 86400);
    AddDHCPOption(packet, offset, DHCP_OPTION_LEASE_TIME, 4, leaseTime);
    packet[offset++] = DHCP_OPTION_END;
    
    // Find and verify
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_LEASE_TIME,
                                 optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 4);
    EXPECT_EQ(Unpack32(packet, optionOffset), 86400);
}

TEST(DHCPOptions, RenewTimeOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 2, 0x55555555);
    
    // Add Renew Time option (43200 seconds = 12 hours)
    uint8_t renewTime[4];
    Pack32(renewTime, 0, 43200);
    AddDHCPOption(packet, offset, DHCP_OPTION_RENEW_TIME, 4, renewTime);
    packet[offset++] = DHCP_OPTION_END;
    
    // Find and verify
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_RENEW_TIME,
                                 optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 4);
    EXPECT_EQ(Unpack32(packet, optionOffset), 43200);
}

TEST(DHCPOptions, RebindTimeOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 2, 0x66666666);
    
    // Add Rebind Time option (75600 seconds = 21 hours)
    uint8_t rebindTime[4];
    Pack32(rebindTime, 0, 75600);
    AddDHCPOption(packet, offset, DHCP_OPTION_REBIND_TIME, 4, rebindTime);
    packet[offset++] = DHCP_OPTION_END;
    
    // Find and verify
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_REBIND_TIME,
                                 optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 4);
    EXPECT_EQ(Unpack32(packet, optionOffset), 75600);
}

TEST(DHCPOptions, BroadcastAddressOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 2, 0x77777777);
    
    // Add Broadcast Address option
    uint8_t broadcast[] = {192, 168, 1, 255};
    AddDHCPOption(packet, offset, DHCP_OPTION_BROADCAST, 4, broadcast);
    packet[offset++] = DHCP_OPTION_END;
    
    // Find and verify
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_BROADCAST,
                                 optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 4);
    EXPECT_EQ(Unpack32(packet, optionOffset), 0xC0A801FF);
}

TEST(DHCPOptions, HostnameOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0x88888888);
    
    // Add Hostname option
    const char* hostname = "tinytcp";
    AddDHCPOption(packet, offset, DHCP_OPTION_HOSTNAME, strlen(hostname),
                  (const uint8_t*)hostname);
    packet[offset++] = DHCP_OPTION_END;
    
    // Find and verify
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_HOSTNAME,
                                 optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 7);
    
    char readHostname[8];
    memcpy(readHostname, &packet[optionOffset], optionLength);
    readHostname[optionLength] = '\0';
    EXPECT_STREQ(readHostname, "tinytcp");
}

TEST(DHCPOptions, ClientIDOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0x99999999);
    
    // Add Client ID option (type + MAC address)
    uint8_t clientID[] = {1, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    AddDHCPOption(packet, offset, DHCP_OPTION_CLIENT_ID, 7, clientID);
    packet[offset++] = DHCP_OPTION_END;
    
    // Find and verify
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_CLIENT_ID,
                                 optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 7);
    EXPECT_EQ(packet[optionOffset], 1);  // Hardware address type
    EXPECT_EQ(packet[optionOffset + 1], 0xAA);
    EXPECT_EQ(packet[optionOffset + 6], 0xFF);
}

TEST(DHCPOptions, ParameterRequestListOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0xAAAAAAAA);
    
    // Add Parameter Request List option
    uint8_t paramList[] = {1, 3, 6, 15};  // Subnet, Router, DNS, Domain
    AddDHCPOption(packet, offset, DHCP_OPTION_PARAM_REQUEST, 4, paramList);
    packet[offset++] = DHCP_OPTION_END;
    
    // Find and verify
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_PARAM_REQUEST,
                                 optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 4);
    EXPECT_EQ(packet[optionOffset], 1);
    EXPECT_EQ(packet[optionOffset + 1], 3);
    EXPECT_EQ(packet[optionOffset + 2], 6);
    EXPECT_EQ(packet[optionOffset + 3], 15);
}

// ============================================================================
// DHCP Multiple Options Tests
// ============================================================================

TEST(DHCPOptions, MultipleOptionsInSequence) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 2, 0xBBBBBBBB);
    
    // Add multiple options
    uint8_t messageType = DHCP_OFFER;
    AddDHCPOption(packet, offset, DHCP_OPTION_MESSAGE_TYPE, 1, &messageType);
    
    uint8_t serverIP[] = {192, 168, 1, 1};
    AddDHCPOption(packet, offset, DHCP_OPTION_SERVER_ID, 4, serverIP);
    
    uint8_t subnetMask[] = {255, 255, 255, 0};
    AddDHCPOption(packet, offset, DHCP_OPTION_SUBNET_MASK, 4, subnetMask);
    
    uint8_t router[] = {192, 168, 1, 1};
    AddDHCPOption(packet, offset, DHCP_OPTION_ROUTER, 4, router);
    
    packet[offset++] = DHCP_OPTION_END;
    
    // Verify all options can be found
    size_t optionOffset;
    uint8_t optionLength;
    
    EXPECT_TRUE(FindDHCPOption(packet, 240, offset, DHCP_OPTION_MESSAGE_TYPE,
                               optionOffset, optionLength));
    EXPECT_TRUE(FindDHCPOption(packet, 240, offset, DHCP_OPTION_SERVER_ID,
                               optionOffset, optionLength));
    EXPECT_TRUE(FindDHCPOption(packet, 240, offset, DHCP_OPTION_SUBNET_MASK,
                               optionOffset, optionLength));
    EXPECT_TRUE(FindDHCPOption(packet, 240, offset, DHCP_OPTION_ROUTER,
                               optionOffset, optionLength));
}

TEST(DHCPOptions, FindNonExistentOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0xCCCCCCCC);
    
    uint8_t messageType = DHCP_DISCOVER;
    AddDHCPOption(packet, offset, DHCP_OPTION_MESSAGE_TYPE, 1, &messageType);
    packet[offset++] = DHCP_OPTION_END;
    
    // Try to find an option that doesn't exist
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, DHCP_OPTION_SERVER_ID,
                                 optionOffset, optionLength);
    
    EXPECT_FALSE(found);
}

TEST(DHCPOptions, EndOptionTerminatesSearch) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0xDDDDDDDD);
    
    uint8_t messageType = DHCP_DISCOVER;
    AddDHCPOption(packet, offset, DHCP_OPTION_MESSAGE_TYPE, 1, &messageType);
    packet[offset++] = DHCP_OPTION_END;
    
    // Add another option after END (should not be found)
    uint8_t serverIP[] = {192, 168, 1, 1};
    AddDHCPOption(packet, offset, DHCP_OPTION_SERVER_ID, 4, serverIP);
    
    // Should find message type
    size_t optionOffset;
    uint8_t optionLength;
    EXPECT_TRUE(FindDHCPOption(packet, 240, offset, DHCP_OPTION_MESSAGE_TYPE,
                               optionOffset, optionLength));
    
    // Should NOT find server ID (after END)
    EXPECT_FALSE(FindDHCPOption(packet, 240, offset, DHCP_OPTION_SERVER_ID,
                                optionOffset, optionLength));
}

// ============================================================================
// DHCP Message Type Tests
// ============================================================================

TEST(DHCPMessage, DiscoverPacketStructure) {
    uint8_t packet[300];
    size_t offset;
    
    // Create a DHCP DISCOVER message
    CreateDHCPPacket(packet, offset, 1, 0x12345678);
    
    uint8_t messageType = DHCP_DISCOVER;
    AddDHCPOption(packet, offset, DHCP_OPTION_MESSAGE_TYPE, 1, &messageType);
    
    const char* hostname = "tinytcp";
    AddDHCPOption(packet, offset, DHCP_OPTION_HOSTNAME, strlen(hostname),
                  (const uint8_t*)hostname);
    
    packet[offset++] = DHCP_OPTION_END;
    
    // Verify it's a valid DISCOVER
    EXPECT_EQ(Unpack8(packet, 0), 1);  // BOOTREQUEST
    
    size_t optionOffset;
    uint8_t optionLength;
    EXPECT_TRUE(FindDHCPOption(packet, 240, offset, DHCP_OPTION_MESSAGE_TYPE,
                               optionOffset, optionLength));
    EXPECT_EQ(packet[optionOffset], DHCP_DISCOVER);
}

TEST(DHCPMessage, OfferPacketStructure) {
    uint8_t packet[300];
    size_t offset;
    
    // Create a DHCP OFFER message
    CreateDHCPPacket(packet, offset, 2, 0xABCDEF00);
    
    // Set offered IP address
    Pack32(packet, 16, 0xC0A80164);  // yiaddr: 192.168.1.100
    
    uint8_t messageType = DHCP_OFFER;
    AddDHCPOption(packet, offset, DHCP_OPTION_MESSAGE_TYPE, 1, &messageType);
    
    uint8_t serverIP[] = {192, 168, 1, 1};
    AddDHCPOption(packet, offset, DHCP_OPTION_SERVER_ID, 4, serverIP);
    
    uint8_t subnetMask[] = {255, 255, 255, 0};
    AddDHCPOption(packet, offset, DHCP_OPTION_SUBNET_MASK, 4, subnetMask);
    
    packet[offset++] = DHCP_OPTION_END;
    
    // Verify OFFER structure
    EXPECT_EQ(Unpack8(packet, 0), 2);  // BOOTREPLY
    EXPECT_EQ(Unpack32(packet, 16), 0xC0A80164);  // Offered IP
    
    size_t optionOffset;
    uint8_t optionLength;
    EXPECT_TRUE(FindDHCPOption(packet, 240, offset, DHCP_OPTION_MESSAGE_TYPE,
                               optionOffset, optionLength));
    EXPECT_EQ(packet[optionOffset], DHCP_OFFER);
}

TEST(DHCPMessage, RequestPacketStructure) {
    uint8_t packet[300];
    size_t offset;
    
    // Create a DHCP REQUEST message
    CreateDHCPPacket(packet, offset, 1, 0x11223344);
    
    uint8_t messageType = DHCP_REQUEST;
    AddDHCPOption(packet, offset, DHCP_OPTION_MESSAGE_TYPE, 1, &messageType);
    
    uint8_t requestedIP[] = {192, 168, 1, 100};
    AddDHCPOption(packet, offset, DHCP_OPTION_REQUESTED_IP, 4, requestedIP);
    
    uint8_t serverIP[] = {192, 168, 1, 1};
    AddDHCPOption(packet, offset, DHCP_OPTION_SERVER_ID, 4, serverIP);
    
    packet[offset++] = DHCP_OPTION_END;
    
    // Verify REQUEST structure
    EXPECT_EQ(Unpack8(packet, 0), 1);  // BOOTREQUEST
    
    size_t optionOffset;
    uint8_t optionLength;
    EXPECT_TRUE(FindDHCPOption(packet, 240, offset, DHCP_OPTION_MESSAGE_TYPE,
                               optionOffset, optionLength));
    EXPECT_EQ(packet[optionOffset], DHCP_REQUEST);
}

TEST(DHCPMessage, AckPacketStructure) {
    uint8_t packet[300];
    size_t offset;
    
    // Create a DHCP ACK message
    CreateDHCPPacket(packet, offset, 2, 0x55667788);
    
    // Set assigned IP address
    Pack32(packet, 16, 0xC0A80164);  // yiaddr: 192.168.1.100
    
    uint8_t messageType = DHCP_ACK;
    AddDHCPOption(packet, offset, DHCP_OPTION_MESSAGE_TYPE, 1, &messageType);
    
    uint8_t serverIP[] = {192, 168, 1, 1};
    AddDHCPOption(packet, offset, DHCP_OPTION_SERVER_ID, 4, serverIP);
    
    uint8_t subnetMask[] = {255, 255, 255, 0};
    AddDHCPOption(packet, offset, DHCP_OPTION_SUBNET_MASK, 4, subnetMask);
    
    uint8_t router[] = {192, 168, 1, 1};
    AddDHCPOption(packet, offset, DHCP_OPTION_ROUTER, 4, router);
    
    uint8_t dns[] = {8, 8, 8, 8};
    AddDHCPOption(packet, offset, DHCP_OPTION_DNS, 4, dns);
    
    uint8_t leaseTime[4];
    Pack32(leaseTime, 0, 86400);
    AddDHCPOption(packet, offset, DHCP_OPTION_LEASE_TIME, 4, leaseTime);
    
    packet[offset++] = DHCP_OPTION_END;
    
    // Verify ACK structure
    EXPECT_EQ(Unpack8(packet, 0), 2);  // BOOTREPLY
    EXPECT_EQ(Unpack32(packet, 16), 0xC0A80164);  // Assigned IP
    
    size_t optionOffset;
    uint8_t optionLength;
    EXPECT_TRUE(FindDHCPOption(packet, 240, offset, DHCP_OPTION_MESSAGE_TYPE,
                               optionOffset, optionLength));
    EXPECT_EQ(packet[optionOffset], DHCP_ACK);
}

TEST(DHCPMessage, NakPacketStructure) {
    uint8_t packet[300];
    size_t offset;
    
    // Create a DHCP NAK message
    CreateDHCPPacket(packet, offset, 2, 0x99AABBCC);
    
    uint8_t messageType = DHCP_NAK;
    AddDHCPOption(packet, offset, DHCP_OPTION_MESSAGE_TYPE, 1, &messageType);
    
    uint8_t serverIP[] = {192, 168, 1, 1};
    AddDHCPOption(packet, offset, DHCP_OPTION_SERVER_ID, 4, serverIP);
    
    packet[offset++] = DHCP_OPTION_END;
    
    // Verify NAK structure
    EXPECT_EQ(Unpack8(packet, 0), 2);  // BOOTREPLY
    
    size_t optionOffset;
    uint8_t optionLength;
    EXPECT_TRUE(FindDHCPOption(packet, 240, offset, DHCP_OPTION_MESSAGE_TYPE,
                               optionOffset, optionLength));
    EXPECT_EQ(packet[optionOffset], DHCP_NAK);
}

// ============================================================================
// DHCP Workflow Sequence Tests
// ============================================================================

TEST(DHCPWorkflow, DiscoverOfferSequence) {
    // Simulate DISCOVER -> OFFER sequence
    uint8_t discoverPacket[300];
    size_t discoverOffset;
    
    // Client sends DISCOVER
    uint32_t xid = 0x12345678;
    CreateDHCPPacket(discoverPacket, discoverOffset, 1, xid);
    
    uint8_t messageType = DHCP_DISCOVER;
    AddDHCPOption(discoverPacket, discoverOffset, DHCP_OPTION_MESSAGE_TYPE, 1, &messageType);
    discoverPacket[discoverOffset++] = DHCP_OPTION_END;
    
    // Server responds with OFFER
    uint8_t offerPacket[300];
    size_t offerOffset;
    
    CreateDHCPPacket(offerPacket, offerOffset, 2, xid);  // Same XID
    Pack32(offerPacket, 16, 0xC0A80164);  // Offer IP
    
    messageType = DHCP_OFFER;
    AddDHCPOption(offerPacket, offerOffset, DHCP_OPTION_MESSAGE_TYPE, 1, &messageType);
    offerPacket[offerOffset++] = DHCP_OPTION_END;
    
    // Verify XIDs match
    EXPECT_EQ(Unpack32(discoverPacket, 4), Unpack32(offerPacket, 4));
}

TEST(DHCPWorkflow, RequestAckSequence) {
    // Simulate REQUEST -> ACK sequence
    uint32_t xid = 0xAABBCCDD;
    
    // Client sends REQUEST
    uint8_t requestPacket[300];
    size_t requestOffset;
    
    CreateDHCPPacket(requestPacket, requestOffset, 1, xid);
    
    uint8_t messageType = DHCP_REQUEST;
    AddDHCPOption(requestPacket, requestOffset, DHCP_OPTION_MESSAGE_TYPE, 1, &messageType);
    
    uint8_t requestedIP[] = {192, 168, 1, 100};
    AddDHCPOption(requestPacket, requestOffset, DHCP_OPTION_REQUESTED_IP, 4, requestedIP);
    requestPacket[requestOffset++] = DHCP_OPTION_END;
    
    // Server responds with ACK
    uint8_t ackPacket[300];
    size_t ackOffset;
    
    CreateDHCPPacket(ackPacket, ackOffset, 2, xid);  // Same XID
    Pack32(ackPacket, 16, 0xC0A80164);  // Assign IP
    
    messageType = DHCP_ACK;
    AddDHCPOption(ackPacket, ackOffset, DHCP_OPTION_MESSAGE_TYPE, 1, &messageType);
    ackPacket[ackOffset++] = DHCP_OPTION_END;
    
    // Verify XIDs match
    EXPECT_EQ(Unpack32(requestPacket, 4), Unpack32(ackPacket, 4));
    
    // Verify assigned IP matches requested IP
    size_t optionOffset;
    uint8_t optionLength;
    FindDHCPOption(requestPacket, 240, requestOffset, DHCP_OPTION_REQUESTED_IP,
                   optionOffset, optionLength);
    uint32_t requested = Unpack32(requestPacket, optionOffset);
    uint32_t assigned = Unpack32(ackPacket, 16);
    EXPECT_EQ(requested, assigned);
}

// ============================================================================
// DHCP Edge Cases and Error Handling
// ============================================================================

TEST(DHCPEdgeCases, EmptyOptionsSection) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0xFFFFFFFF);
    
    // Immediately add END option (no other options)
    packet[offset++] = DHCP_OPTION_END;
    
    // Should not find any options
    size_t optionOffset;
    uint8_t optionLength;
    EXPECT_FALSE(FindDHCPOption(packet, 240, offset, DHCP_OPTION_MESSAGE_TYPE,
                                optionOffset, optionLength));
}

TEST(DHCPEdgeCases, MaximumOptionLength) {
    uint8_t packet[1500];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0x11111111);
    
    // Add an option with maximum allowed length (255)
    uint8_t largeData[255];
    memset(largeData, 0xAA, 255);
    
    packet[offset++] = 99;  // Vendor specific option
    packet[offset++] = 255;  // Max length
    memcpy(&packet[offset], largeData, 255);
    offset += 255;
    
    packet[offset++] = DHCP_OPTION_END;
    
    // Verify it can be found
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, 99, optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 255);
}

TEST(DHCPEdgeCases, ZeroLengthOption) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0x22222222);
    
    // Add an option with zero length
    packet[offset++] = 100;  // Custom option
    packet[offset++] = 0;    // Zero length
    
    packet[offset++] = DHCP_OPTION_END;
    
    // Should find it with zero length
    size_t optionOffset;
    uint8_t optionLength;
    bool found = FindDHCPOption(packet, 240, offset, 100, optionOffset, optionLength);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(optionLength, 0);
}

TEST(DHCPEdgeCases, InvalidMagicCookie) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0x33333333);
    
    // Corrupt the magic cookie
    Pack32(packet, 236, 0xDEADBEEF);
    
    // Verify it's corrupted
    EXPECT_NE(Unpack32(packet, 236), DHCP_MAGIC_COOKIE);
}

TEST(DHCPEdgeCases, TransactionIDPreservation) {
    // Test that XID is preserved in request/reply
    uint32_t originalXID = 0x87654321;
    
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, originalXID);
    
    // Read back the XID
    uint32_t readXID = Unpack32(packet, 4);
    EXPECT_EQ(readXID, originalXID);
}

TEST(DHCPEdgeCases, BroadcastFlagSet) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0x44444444);
    
    // Verify broadcast flag is set (bit 15 of flags field)
    uint16_t flags = Unpack16(packet, 10);
    EXPECT_EQ(flags & 0x8000, 0x8000);
}

TEST(DHCPEdgeCases, UnusedFieldsZeroed) {
    uint8_t packet[300];
    size_t offset;
    
    CreateDHCPPacket(packet, offset, 1, 0x55555555);
    
    // Verify sname field is zeroed (64 bytes at offset 44)
    for (int i = 0; i < 64; i++) {
        EXPECT_EQ(packet[44 + i], 0);
    }
    
    // Verify file field is zeroed (128 bytes at offset 108)
    for (int i = 0; i < 128; i++) {
        EXPECT_EQ(packet[108 + i], 0);
    }
}
