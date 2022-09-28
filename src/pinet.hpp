//
//  pinet.hpp
//  pinet
//
//  Created by Shine Chang on 9/23/22.
//

#ifndef pinet_hpp
#define pinet_hpp

#include <stdio.h>
#include <map>
#include <string>

class Pinet {
private:
    std::map<std::string, int> tcp_active_sockets;
    int raw_socket;
    int tcp_listen_socket;
    int tcp_socket;
    const int TCP_LISTEN_PORT;
    const std::string USERNAME;
    
public:
    Pinet(std::string username, int listen_port);
    
    int raw_start_socket();
    std::string raw_await();
    int raw_broadcast();

    int tcp_start_socket();
    int tcp_connect_to(std::string peer);
    int tcp_send_to(std::string peer, std::string message);
    std::string tcp_await();
};

#endif /* pinet_hpp */
