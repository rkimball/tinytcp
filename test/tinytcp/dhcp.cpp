// #include <gtest/gtest.h>
// #include <cstring>
// #include "data_buffer.hpp"
// #include "utility.hpp"
// #include "dhcp.hpp"
// // #include "interface_mac.hpp"
// #include "ipv4.hpp"
// #include "udp.hpp"

// using namespace tinytcp;

// // Add a mock MAC, IP, and UDP class if necessary for testing ProtocolDHCP
// class MockMAC : public InterfaceMAC {
// public:
//     const uint8_t* GetUnicastAddress() const override {
//         static uint8_t addr[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
//         return addr;
//     }
//     const uint8_t* GetBroadcastAddress() const override {
//         static uint8_t addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//         return addr;
//     }
//     size_t AddressSize() const override { return 6; }
//     size_t HeaderSize() const override { return 14; }
//     DataBuffer* GetTxBuffer() override { return new DataBuffer(512); }
//     void FreeTxBuffer(DataBuffer* buffer) override { delete buffer; }
//     void FreeRxBuffer(DataBuffer* buffer) override { delete buffer; }
//     void Transmit(DataBuffer* buffer, const uint8_t* targetMAC, uint16_t type) override {
//         // Store the buffer for verification in tests
//         transmittedBuffer = buffer;
//     }
//     void Retransmit(DataBuffer* buffer) override {}
//     void RegisterDataTransmitHandler(DataTransmitHandler) override {}

//     DataBuffer* transmittedBuffer = nullptr;
// };

// class MockIPv4 : public ProtocolIPv4 {
//     // override SetAddressInfo and GetUnicastAddress.
// public:
//     void SetAddressInfo(const uint8_t* address, const uint8_t* netmask, const uint8_t* gateway) override {
//         std::memcpy(unicastAddress, address, 4);
//     }
//     const uint8_t* GetUnicastAddress() const override {
//         return unicastAddress;
//     }
// private:
//     uint8_t unicastAddress[4] = {0, 0, 0, 0};
// };

// TEST(dhcp, discover_packet_construction) {
//     MockMAC mac;
//     MockIPv4 ip;
//     ProtocolUDP udp;
//     ProtocolDHCP dhcp(mac, ip, udp);

//     dhcp.Discover();

//     // Verify that the transmitted packet is as expected
//     DataBuffer* transmittedBuffer = mac.transmittedBuffer;

//     ASSERT_NE(transmittedBuffer, nullptr);

//     // Verify DHCP header fields
//     EXPECT_EQ(Unpack8(transmittedBuffer->Packet, 0), 1);    // op: BOOTREQUEST
//     EXPECT_EQ(Unpack8(transmittedBuffer->Packet, 1), 1);    // htype: Ethernet
//     EXPECT_EQ(Unpack8(transmittedBuffer->Packet, 2), 6);    // hlen: 6 bytes
//     EXPECT_EQ(Unpack8(transmittedBuffer->Packet, 3), 0);    // hops: 0

//     // Verify XID is non-zero
//     uint32_t xid = Unpack32(transmittedBuffer->Packet, 4);
//     EXPECT_NE(xid, 0);

//     // Verify flags (broadcast bit should be set)
//     uint16_t flags = Unpack16(transmittedBuffer->Packet, 10);
//     EXPECT_EQ(flags, 0x8000);

//     // Verify IP addresses are zero (client doesn't have an IP yet)
//     EXPECT_EQ(Unpack32(transmittedBuffer->Packet, 12), 0);  // ciaddr
//     EXPECT_EQ(Unpack32(transmittedBuffer->Packet, 16), 0);  // yiaddr
//     EXPECT_EQ(Unpack32(transmittedBuffer->Packet, 20), 0);  // siaddr
//     EXPECT_EQ(Unpack32(transmittedBuffer->Packet, 24), 0);  // giaddr

//     // Verify client hardware address (chaddr) matches MAC address
//     const uint8_t* macAddr = mac.GetUnicastAddress();
//     for (int i = 0; i < 6; i++) {
//         EXPECT_EQ(transmittedBuffer->Packet[28 + i], macAddr[i]);
//     }

//     // Verify remaining chaddr bytes are zero-padded
//     for (int i = 6; i < 16; i++) {
//         EXPECT_EQ(transmittedBuffer->Packet[28 + i], 0);
//     }

//     // Verify magic cookie at offset 236
//     uint32_t magic = Unpack32(transmittedBuffer->Packet, 236);
//     EXPECT_EQ(magic, 0x63825363);

//     // Verify DHCP options section starts at offset 240
//     // Look for DHCP Message Type option (53)
//     bool foundMessageType = false;
//     bool foundClientID = false;
//     bool foundHostname = false;
//     bool foundParamRequest = false;
    
//     size_t offset = 240;
//     while (offset < transmittedBuffer->Length) {
//         uint8_t option = Unpack8(transmittedBuffer->Packet, offset++);
        
//         if (option == 255) {  // End option
//             break;
//         }
        
//         uint8_t length = Unpack8(transmittedBuffer->Packet, offset++);
        
//         if (option == 53) {  // DHCP Message Type
//             foundMessageType = true;
//             EXPECT_EQ(length, 1);
//             EXPECT_EQ(Unpack8(transmittedBuffer->Packet, offset), 1);  // 1 = DHCPDISCOVER
//         } else if (option == 61) {  // Client Identifier
//             foundClientID = true;
//             EXPECT_EQ(length, 7);  // 1 byte type + 6 byte MAC
//             EXPECT_EQ(Unpack8(transmittedBuffer->Packet, offset), 1);  // Hardware address type
//             // Verify MAC address in client ID
//             for (int i = 0; i < 6; i++) {
//                 EXPECT_EQ(Unpack8(transmittedBuffer->Packet, offset + 1 + i), macAddr[i]);
//             }
//         } else if (option == 12) {  // Hostname
//             foundHostname = true;
//             EXPECT_EQ(length, 7);  // "tinytcp"
//             char hostname[8];
//             for (int i = 0; i < length; i++) {
//                 hostname[i] = transmittedBuffer->Packet[offset + i];
//             }
//             hostname[length] = '\0';
//             EXPECT_STREQ(hostname, "tinytcp");
//         } else if (option == 55) {  // Parameter Request List
//             foundParamRequest = true;
//             EXPECT_EQ(length, 4);
//             EXPECT_EQ(Unpack8(transmittedBuffer->Packet, offset), 1);   // Subnet mask
//             EXPECT_EQ(Unpack8(transmittedBuffer->Packet, offset + 1), 3);   // Router
//             EXPECT_EQ(Unpack8(transmittedBuffer->Packet, offset + 2), 6);   // DNS
//             EXPECT_EQ(Unpack8(transmittedBuffer->Packet, offset + 3), 15);  // Domain name
//         }
        
//         offset += length;
//     }

//     // Verify all expected options were found
//     EXPECT_TRUE(foundMessageType) << "DHCP Message Type option (53) not found";
//     EXPECT_TRUE(foundClientID) << "Client Identifier option (61) not found";
//     EXPECT_TRUE(foundHostname) << "Hostname option (12) not found";
//     EXPECT_TRUE(foundParamRequest) << "Parameter Request List option (55) not found";
// }