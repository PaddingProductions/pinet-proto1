//
//  main.cpp
//  pinet
//
//  Created by Shine Chang on 9/23/22.
//

#include <iostream>
#include "pinet.hpp"

int main(int argc, const char * argv[]) {
    printf("Input username: ");
    std::string username;
    std::cin >> username;
    printf("\n");
    
    printf("Starting program as '%s'...\n", username.c_str());
    Pinet pinet = Pinet(username, 8080);
    // Enter main loop
    while (true) {
        std::string command;
        std::cin >> command;
        if (!command.compare("quit")) {
            printf("Ending...\n");
            return 0;
        } else if (!command.compare("tcp-start-listen")) {
            pinet.tcp_start_socket();
        } else if (!command.compare("raw-start")) {
            pinet.raw_start_socket();
        } else if (!command.compare("raw-broadcast")) {
            pinet.raw_broadcast();
        } else if (!command.compare("raw-await")) {
            std::cout << pinet.raw_await() << std::endl;
        } else if (!command.compare("tcp-connect")) {
            std::string peer;
            std::cin >> peer;
            pinet.tcp_connect_to(peer);
        } else if (!command.compare("tcp-send")) {
            std::string peer, message;
            std::cin >> peer >> message;
            pinet.tcp_send_to(peer, message);
        } else if (!command.compare("tcp-await")) {
            std::string message = pinet.tcp_await();
            std::cout << message << std::endl;
        } else {
            printf("Unknown command\n");
            continue;
        }
        printf("Done.\n");
    }
    return 0;
}
