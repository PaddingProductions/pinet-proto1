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
#include <list>

struct Pinet::broadcast_hdr {
    unsigned int payload_size;
    unsigned int ip_addr;
    unsigned short tcp_listen_port;
};

std::vector<std::string> args split_message(std::string str, int length) {
    std::vector<std::string> args (3);
    int prev, pos = -1;

    for (int i=0; i<length; i++) { 
        pos = str.find(';', p);
        if (pos != std::string::npos) 
            break;
        args[i] = str.substr(prev+1, i);
        prev = pos;    
    }
    return args;
}

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

char* Pinet::raw_await() {
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

        // Check identifier (Ethertype) (filter out non-pinet protocl packets)
	    if (eth->h_proto[0] != 0x00 || pinet_hdr->h_proto[1] != 0x00 || pinet_hdr->h_proto[2] != 0xff  || pinet_hdr->h_proto[3] != 0xff)
		    continue;

        // Extract Pinet header
        pinethdr* pinet_hdr = (pinethdr*)(packet + sizeof(ethhdr));

	    // PINET HEADER - Fetch size of payload        
	    unsigned int payload_size = ntohl(pinet_hdr->payload_size);
	    unsigned int payload_len = packet_len - sizeof(pinethdr) - sizeof(ethhdr);
 
	    // Print out Ethernet header
        printf("\nPinet L2 broadcast received: Ethernet Header:\n");
        printf("\t|-Source Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
        printf("\t|-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
        printf("\t|-Protocol : %d\n",eth->h_proto);
	    printf("\t|-Packet size (raw): %d\n", packet_len); 

        return (char*) pinet_hdr;
    }
}

int Pinet::L2_broadcast_process (char* raw_packet) {
    PINET::broadcast_hdr* hdr = (Pinet::broadcast_hdr*) raw_packet;

    // Extract Header Info
    u_int32_t peer_ip_addr = ntohl(hdr->ip_addr);
    u_int32_t payload_size = ntohl(hdr->payload_size);
     
    // Extract tcp listen port from header.
    u_int16_t port = ntohs(hdr->tcp_listen_port);

    // Extract username from payload, terminated by \0
    char* payload = raw_packet + sizeof(Pinet::broadcast_hdr);
    unsigned int name_len = 0;
    while (name_len<payload_len && payload[name_len] != '\0') name_len++;
    std::string peer_name = std::string(packet, name_len);

    // Extract broadcast message from payload, terminated by \0
    char* message = payload + name_len;
    unsigned int message_len = payload_size - name_len;
    std::string message = (payload, message_len);
   

    // Log broadcast packets 
    char ip_addr_str [INET_ADDRSTRLEN];
    inet_pton(AF_INET, sin->sin_addr, ip_addr_str, sizeof (ip_addr_str));

    printf("\nBroadcast data:\n");
    printf("\t|-Node Address: %s:%d\n", ip_addr_str, peer_listen_port);
    printf("\t|-Node Name: %s\n", peer_name);   
    printf("\t|-Broadcasted Message: %s\n", message);

    return 0;
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
    // Construct Payload (Message)
    std::string message = USERNAME + '\0' + "Hello PINET from " + USERNAMEi + "!\0";
    

    // get the index number of the current interface 
    const char *interface_name = "wlan0"; 
    ifreq ifr = {0};
    strncpy(ifr.ifr_name, interface_name, strlen(interface_name));
    if (ioctl(raw_socket, SIOCGIFINDEX, &ifr) < 0) 
        printf("unable to get index of interface\n");   
    // get IP address of the interface
    if (ioctl(raw_socket, SIOCGIFADDR, &ifr) < 0) {
        printf("unable to get index of interface\n");   
    }
	// get MAC address of the interface
	if (ioctl(raw_socket, SIOCGIFHWADDR, &if_mac) < 0)
	    perror("SIOCGIFHWADDR");
    
    // construct address 
    sockaddr_ll dest_addr;
     
    dest_addr.sll_family = AF_PACKET;    
    dest_addr.sll_pkttype = (PACKET_BROADCAST);
    dest_addr.sll_ifindex = ifr->ifindex;    
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
    eth_hdr->h_source[0] = ((uint8_t *)&ifr.ifr_hwaddr.sa_data)[0];
	eth_hdr->h_source[1] = ((uint8_t *)&ifr.ifr_hwaddr.sa_data)[1];
	eth_hdr->h_source[2] = ((uint8_t *)&ifr.ifr_hwaddr.sa_data)[2];
	eth_hdr->h_source[3] = ((uint8_t *)&ifr.ifr_hwaddr.sa_data)[3];
	eth_hdr->h_source[4] = ((uint8_t *)&ifr.ifr_hwaddr.sa_data)[4];
	eth_hdr->h_source[5] = ((uint8_t *)&ifr.ifr_hwaddr.sa_data)[5];
    
    // Write FF:FF:FF:FF (broadcast MAC address) to ethernet header 
    eth_hdr->h_dest[0] = 0xff;
    eth_hdr->h_dest[1] = 0xff;
    eth_hdr->h_dest[2] = 0xff;
    eth_hdr->h_dest[3] = 0xff;
    eth_hdr->h_dest[4] = 0xff;
    eth_hdr->h_dest[5] = 0xff;
    
    eth_hdr->h_proto = htons(0x00ff);//htons(ETH_P_IP);

    // PINET header (first four bytes are 'ff')
    Pinet::broadcast_hdr* pinet_hdr = (Pinet::broadcast_hdr*)(packet + sizeof(ethhdr));
    /*
    pinet_hdr->identifier[0] = 0xff;
    pinet_hdr->identifier[1] = 0xff;
    pinet_hdr->identifier[2] = 0xff;
    pinet_hdr->identifier[3] = 0xff;
    */

    // PINET header - size
    int payload_size = message.size();
    pinet_hdr->payload_size = htonl(payload_size);

    // PINET header - ip, in bytes
    pinet_hdr->ip_addr = inet_aton(ip_bytes, ifr->if_addr);

    // PINET header - port, in bytes
    pinet_hdr->tcp_listen_port = htons(TCP_LISTEN_PORT);

    // Write message 
    char* payload = (char *)pinet_hdr + sizeof(Pinet::broadcast_hdr);
    memcpy(payload, message.c_str(), message.size());
    
    // Send (Broadcast) packet
    if (sendto(raw_socket, packet, sizeof(packet), 0, (sockaddr *)&dest_addr, sizeof(sockaddr_ll)) < 0) {
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
