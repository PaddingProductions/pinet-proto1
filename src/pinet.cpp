//
//  pinet.cpp
//  pinet
//
//  Created by Shine Chang on 9/23/22.
//

#include "pinet.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <iostream>
#include <linux/if_packet.h>
#include <net/ethernet.h> 
#include <cstring>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ip.h>

struct pinethdr {
    unsigned char identifier[4];
};

Pinet::Pinet (std::string username, int listen_port) : USERNAME(username), TCP_LISTEN_PORT(listen_port) {};

int Pinet::tcp_start_socket() {
    
    printf("creating tcp listen socket...\n");
    
    struct sockaddr_in address;
    int addrlen = sizeof(address);
 
    // Creating socket file descriptor
    if ((tcp_listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        return -1;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(TCP_LISTEN_PORT);
 
    // Forcefully attaching socket to the port 8080
    if (bind(tcp_listen_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return -1;
    }
    
    // begin listening for new tcp connections.
    printf("listening for connection on listen socket...\n");
    if (listen(tcp_listen_socket, 3) < 0) {
        perror("listen");
        return -1;
    }
    // form new connection
    int sock;
    if ((sock = accept(tcp_listen_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        return -1;
    }
    printf("Connection accepted.\n");
    tcp_socket = sock;
    
    return 0;
}
int Pinet::raw_start_socket() {
    
    printf("creating raw link-layer socket...\n");
    if ((raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("on raw socket creation");
        return -1;
    }
    return 0;
}

std::string Pinet::raw_await() {
    //Receive a network packet and copy in to buffer
    char packet[1024] = { 0 };
    sockaddr source_addr;
    socklen_t source_addr_len = sizeof(source_addr);
    
    while (true) {
        // Fetch packet
        int packet_len = (int) recvfrom(raw_socket, packet, 1024, 0, &source_addr, &source_addr_len);
        if(packet_len < 0) {
            printf("error in reading recvfrom function at await_raw_socket().\n");
            return "";
        }
        // Extract Ethernet header 
        ethhdr *eth = (ethhdr *)(packet);

        // Extract Pinet header
        pinethdr* pinet_hdr = (pinethdr*) packet + sizeof(ethhdr);

        // Check identifier (to filter out non-pinet packets)
	// Debug: print out header
	printf("Receieved Packet - Header: ");
	for (int i=0; i<4; i++) 
		printf("%.2X:", pinet_hdr->identifier[i]);
	printf("\n");
	if (pinet_hdr->identifier[0] != 0xff || pinet_hdr->identifier[1] != 0xff || pinet_hdr->identifier[2] != 0xff  || pinet_hdr->identifier[3] != 0xff) 
            continue;
        
        // Extract Payload
        char* payload = (char*) (payload + sizeof(pinethdr));
        // Print out Ethernet header
        printf("\nEthernet Header\n");
        printf("\t|-Source Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
        printf("\t|-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
        printf("\t|-Protocol : %d\n",eth->h_proto);

        return std::string(payload);
    }
}


int Pinet::tcp_connect_to(std::string peer) {
    return 0;
};

int Pinet::tcp_send_to(std::string peer, std::string message) {
    printf("sending '%s'...\n", message.c_str());
    send(tcp_socket, message.c_str(), message.size(), 0);
    
    return 0;
}
int Pinet::raw_broadcast() {
    std::string message = "Hello Wireshark from PI!!!";
    

    // get the index number of the current interface 
    const char *interface_name = "wlan0"; 
    ifreq ifr = {0};
    strncpy(ifr.ifr_name, interface_name, strlen(interface_name));
    if (ioctl(raw_socket, SIOCGIFINDEX, &ifr) < 0) {
        printf("unable to get index of interface\n");   
    }
    int ifindex = ifr.ifr_ifindex;

	// get the MAC address of the interface
    ifreq if_mac = {0};
	strncpy(if_mac.ifr_name, interface_name, strlen(interface_name));
	if (ioctl(raw_socket, SIOCGIFHWADDR, &if_mac) < 0)
	    perror("SIOCGIFHWADDR");
    
    // construct address 
    sockaddr_ll dest_addr;
     
    dest_addr.sll_family = AF_PACKET;    
    dest_addr.sll_pkttype = (PACKET_BROADCAST);
    dest_addr.sll_ifindex = ifindex;    
    dest_addr.sll_halen = ETH_ALEN;
    dest_addr.sll_addr[0] = 0xff;
    dest_addr.sll_addr[1] = 0xff;
    dest_addr.sll_addr[2] = 0xff;
    dest_addr.sll_addr[3] = 0xff;
    dest_addr.sll_addr[4] = 0xff;
    dest_addr.sll_addr[5] = 0xff;

    // Fill in packet
    char packet[1024] = {0};

    // Fill in ethernet header 
    ethhdr* eth_hdr = (ethhdr*)packet;

    // Write interface MAC address to ethernet header 
    eth_hdr->h_source[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	eth_hdr->h_source[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	eth_hdr->h_source[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	eth_hdr->h_source[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	eth_hdr->h_source[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	eth_hdr->h_source[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
    
    // Write FF:FF:FF:FF (broadcast MAC address) to ethernet header 
    eth_hdr->h_dest[0] = 0xff;
    eth_hdr->h_dest[1] = 0xff;
    eth_hdr->h_dest[2] = 0xff;
    eth_hdr->h_dest[3] = 0xff;
    eth_hdr->h_dest[4] = 0xff;
    eth_hdr->h_dest[5] = 0xff;
    
    eth_hdr->h_proto = htons(ETH_P_IP);

    // PINET header (first four bytes are 'ff')
    pinethdr* pinet_hdr = (pinethdr*)(packet + sizeof(ethhdr));
    pinet_hdr->identifier[0] = 0xff;
    pinet_hdr->identifier[1] = 0xff;
    pinet_hdr->identifier[2] = 0xff;
    pinet_hdr->identifier[3] = 0xff;

    // Write message 
    char* payload = (char *)pinet_hdr + sizeof(pinethdr);
    memcpy(payload, message.c_str(), message.size());
    
    // Send (Broadcast) packet
    if (sendto(raw_socket, packet, sizeof(packet),  0, (sockaddr *)&dest_addr, sizeof(sockaddr_ll)) < 0) {
        std::cout << errno << std::endl;
        perror("Failed to send broadcast message"); 
        return -1;
    }
    
    return 0;
};
std::string Pinet::tcp_await() {
    char buffer[1024];
    int valread = (int) read(tcp_socket, buffer, 1024);
    
    return std::string(buffer);
}
