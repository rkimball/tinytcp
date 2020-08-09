//----------------------------------------------------------------------------
// Copyright(c) 2015-2020, Robert Kimball
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//----------------------------------------------------------------------------

#ifdef _WIN32
#include <Packet32.h>
#include <ntddndis.h>
#include <pcap.h>
#include <winsock.h>
#elif __linux__
#include <errno.h>
#include <ifaddrs.h>
#include <linux/icmp.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif
#include <cstring>
#include <stdio.h>

#include "InterfaceMAC.hpp"
#include "PacketIO.hpp"
#include "Utility.hpp"

#define Max_Num_Adapter 10
char AdapterList[Max_Num_Adapter][1024];

#define IPTOSBUFFERS 12
char* iptos(u_long in)
{
    static char output[IPTOSBUFFERS][3 * 4 + 3 + 1];
    static short which;
    u_char* p;

    p = (u_char*)&in;
    which = (which + 1 == IPTOSBUFFERS ? 0 : which + 1);
    snprintf(output[which], sizeof(output[which]), "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
    return output[which];
}

char* ip6tos(struct sockaddr* sockaddr, char* address, int addrlen)
{
    socklen_t sockaddrlen;

#ifdef WIN32
    sockaddrlen = sizeof(struct sockaddr_in6);
#else
    sockaddrlen = sizeof(struct sockaddr_storage);
#endif

    if (getnameinfo(sockaddr, sockaddrlen, address, addrlen, NULL, 0, NI_NUMERICHOST) != 0)
        address = NULL;

    return address;
}

#ifdef _WIN32
int PacketIO::GetMACAddress(const char* adapter, uint8_t* mac)
{
    LPADAPTER lpAdapter = 0;
    int i;
    DWORD dwErrorCode;
    char AdapterName[8192];
    char *temp, *temp1;
    int AdapterNum = 0, Open;
    ULONG AdapterLength;
    PPACKET_OID_DATA OidData;
    BOOLEAN Status;

    lpAdapter = PacketOpenAdapter((char*)adapter);

    if (!lpAdapter || (lpAdapter->hFile == INVALID_HANDLE_VALUE))
    {
        dwErrorCode = GetLastError();
        printf("Unable to open the adapter, Error Code : %lx\n", dwErrorCode);

        return -1;
    }

    //
    // Allocate a buffer to get the MAC adress
    //

    OidData = (PACKET_OID_DATA*)malloc(6 + sizeof(PACKET_OID_DATA));
    if (OidData == NULL)
    {
        printf("error allocating memory!\n");
        PacketCloseAdapter(lpAdapter);
        return -1;
    }

    //
    // Retrieve the adapter MAC querying the NIC driver
    //

    OidData->Oid = OID_802_3_CURRENT_ADDRESS;

    OidData->Length = 6;
    ZeroMemory(OidData->Data, 6);

    Status = PacketRequest(lpAdapter, FALSE, OidData);
    if (Status)
    {
        mac[0] = (OidData->Data)[0];
        mac[1] = (OidData->Data)[1];
        mac[2] = (OidData->Data)[2];
        mac[3] = (OidData->Data)[3];
        mac[4] = (OidData->Data)[4];
        mac[5] = (OidData->Data)[5];
        printf("The MAC address of the adapter is %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
               (OidData->Data)[0],
               (OidData->Data)[1],
               (OidData->Data)[2],
               (OidData->Data)[3],
               (OidData->Data)[4],
               (OidData->Data)[5]);
    }
    else
    {
        printf("error retrieving the MAC address of the adapter!\n");
    }

    free(OidData);
    PacketCloseAdapter(lpAdapter);
    return (0);
}

void PacketIO::DisplayDevices()
{
    pcap_if_t* alldevs;
    pcap_if_t* d;
    pcap_addr_t* a;
    int i = 0;
    char errbuf[PCAP_ERRBUF_SIZE];
    char ip6str[128];

    /* Retrieve the device list from the local machine */

    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        fprintf(stderr, "Error in pcap_findalldevs_ex: %s\n", errbuf);
        exit(1);
    }

    /* Print the list */
    for (d = alldevs; d != NULL; d = d->next)
    {
        printf("%d. %s", ++i, d->name);
        if (d->description)
        {
            printf(" (%s)\n", d->description);
        }
        else
        {
            printf(" (No description available)\n");
        }

        for (a = d->addresses; a; a = a->next)
        {
            printf("\tAddress Family: #%d\n", a->addr->sa_family);

            switch (a->addr->sa_family)
            {
            case AF_INET:
                printf("\tAddress Family Name: AF_INET\n");
                if (a->addr)
                    printf("\tAddress: %s\n",
                           iptos(((struct sockaddr_in*)a->addr)->sin_addr.s_addr));
                if (a->netmask)
                    printf("\tNetmask: %s\n",
                           iptos(((struct sockaddr_in*)a->netmask)->sin_addr.s_addr));
                if (a->broadaddr)
                    printf("\tBroadcast Address: %s\n",
                           iptos(((struct sockaddr_in*)a->broadaddr)->sin_addr.s_addr));
                if (a->dstaddr)
                    printf("\tDestination Address: %s\n",
                           iptos(((struct sockaddr_in*)a->dstaddr)->sin_addr.s_addr));
                break;

            case AF_INET6:
                printf("\tAddress Family Name: AF_INET6\n");
                if (a->addr)
                    printf("\tAddress: %s\n", ip6tos(a->addr, ip6str, sizeof(ip6str)));
                break;

            default: printf("\tAddress Family Name: Unknown\n"); break;
            }
        }
    }

    if (i == 0)
    {
        printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
        return;
    }

    /* We don't need any more the device list. Free it */
    pcap_freealldevs(alldevs);
}

void PacketIO::GetDevice(int interfaceNumber, char* buffer, size_t buffer_size)
{
    pcap_if_t* alldevs;
    pcap_if_t* d;
    int i = 0;
    char errbuf[PCAP_ERRBUF_SIZE];

    /* Retrieve the device list from the local machine */

    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        fprintf(stderr, "Error in pcap_findalldevs_ex: %s\n", errbuf);
        exit(1);
    }

    /* Print the list */
    d = alldevs;
    for (int i = 0; i < interfaceNumber - 1; i++)
    {
        if (d)
        {
            d = d->next;
        }
    }
    buffer[0] = 0;
    if (d)
    {
        strncpy(buffer, d->name, buffer_size);
    }

    /* We don't need any more the device list. Free it */
    pcap_freealldevs(alldevs);
}

PacketIO::PacketIO(const char* name)
    : CaptureDevice(name)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    /* Open the device */
    /* Open the adapter */
    if ((adhandle =
             pcap_open_live(CaptureDevice, // name of the device
                            65536,         // portion of the packet to capture.
                            // 65536 grants that the whole packet will be captured on all the MACs.
                            1,     // promiscuous mode (nonzero means promiscuous)
                            1,     // read timeout
                            errbuf // error buffer
                            )) == NULL)
    {
        fprintf(stderr,
                "\nUnable to open the adapter. %s is not supported by WinPcap\n",
                CaptureDevice);
    }
}

//============================================================================
//
//============================================================================

void PacketIO::Start(pcap_handler handler)
{
    /* start the capture */
    pcap_loop(adhandle, 0, handler, NULL);
}

//============================================================================
//
//============================================================================

void PacketIO::Stop()
{
    pcap_close(adhandle);
}

//============================================================================
//
//============================================================================

void PacketIO::TxData(void* packet, size_t length)
{
    //printf( "Ethernet Tx:\n" );
    //DumpData( packet, length, printf );

    if (pcap_sendpacket(adhandle, (u_char*)packet, length) != 0)
    {
        fprintf(stderr, "\nError sending the packet: %s\n", pcap_geterr(adhandle));
    }
}
#elif __linux__

PacketIO::PacketIO() {}

//============================================================================
//
//============================================================================

void PacketIO::DisplayDevices()
{
    struct ifaddrs* ifaddr;

    if (getifaddrs(&ifaddr) == 0)
    {
        for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL)
            {
                continue;
            }
            int family = ifa->ifa_addr->sa_family;

            /* Display interface name and family (including symbolic
                form of the latter for the common families) */

            printf("%-8s %s (%d)\n",
                   ifa->ifa_name,
                   (family == AF_PACKET)
                       ? "AF_PACKET"
                       : (family == AF_INET) ? "AF_INET"
                                             : (family == AF_INET6) ? "AF_INET6" : "???",
                   family);
        }
        freeifaddrs(ifaddr);
    }
}

//============================================================================
//
//============================================================================

void PacketIO::GetInterface(char* name)
{
    struct ifaddrs* ifaddr;

    if (getifaddrs(&ifaddr) == 0)
    {
        for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL)
            {
                continue;
            }
            int family = ifa->ifa_addr->sa_family;

            if (family == AF_PACKET)
            {
                if (strcmp(ifa->ifa_name, "lo") != 0)
                {
                    // yes, this is unsafe
                    strcpy(name, ifa->ifa_name);
                    break;
                }
            }

            // /* Display interface name and family (including symbolic
            //     form of the latter for the common families) */

            // printf("%-8s %s (%d)\n",
            //         ifa->ifa_name,
            //         (family == AF_PACKET) ? "AF_PACKET" :
            //         (family == AF_INET) ? "AF_INET" :
            //         (family == AF_INET6) ? "AF_INET6" : "???",
            //         family);
        }
        freeifaddrs(ifaddr);
    }
}

//============================================================================
//
//============================================================================

void PacketIO::Start(RxDataHandler rxData)
{
    m_RawSocket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (m_RawSocket == -1)
    {
        if (errno == EPERM)
        {
            printf("need root privileges, try 'sudo ./testApp'\n");
            exit(-1);
        }
        else
        {
            printf("Error while creating socket. Aborting...\n");
        }
    }
    else
    {
        struct ifreq ifr;
        PacketIO::GetInterface(ifr.ifr_name);
        printf("Using interface '%s'\n", ifr.ifr_name);

        // Find the socket index for tx later
        if (ioctl(m_RawSocket, SIOCGIFINDEX, &ifr) == -1)
        {
            printf("oh crap %s\n", strerror(errno));
        }
        m_IfIndex = ifr.ifr_ifindex;

        // Set socket to promiscuous mode
        struct packet_mreq mreq;
        mreq.mr_ifindex = ifr.ifr_ifindex;
        mreq.mr_type = PACKET_MR_PROMISC;
        mreq.mr_alen = 6;
        if (setsockopt(
                m_RawSocket, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, (socklen_t)sizeof(mreq)) < 0)
        {
            printf("promiscuous membership error %s", strerror(errno));
        }

        void* pkt_data = (void*)malloc(ETH_FRAME_LEN);
        while (1)
        {
            int length = recvfrom(m_RawSocket, pkt_data, ETH_FRAME_LEN, 0, NULL, NULL);
            rxData((uint8_t*)pkt_data, length);
        }
        free(pkt_data); // no way to get here, but ...
    }
}

//============================================================================
//
//============================================================================

void PacketIO::Stop() {}

//============================================================================
//
//============================================================================

void PacketIO::TxData(void* packet, size_t length)
{
    //   printf( "Ethernet Tx:\n" );
    //   DumpData( packet, length, printf );

    struct sockaddr_ll dest;

    memset(&dest, 0, sizeof(dest));
    dest.sll_family = AF_PACKET;
    dest.sll_ifindex = m_IfIndex;

    int rc = sendto(m_RawSocket, packet, length, 0, (sockaddr*)&dest, sizeof(dest));
    if (rc < 0)
    {
        printf("tx error %s\n", strerror(errno));
    }
}

#endif
