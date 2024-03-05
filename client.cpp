#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <netdb.h> 

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <HOSTNAME-OR-IP> <PORT> <FILENAME>" << std::endl;
        return 1;
    }

    const char* hostname = argv[1];
    const char* port = argv[2];
    const char* filename = argv[3];

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE; 
    hints.ai_protocol = 0; 

    int status;
    if ((status = getaddrinfo(hostname, port, &hints, &res)) != 0) {
        std::cerr << "ERROR: " << gai_strerror(status) << std::endl;
        return 2;
    }

    // Make a socket:
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        std::cerr << "ERROR: " << strerror(errno) << std::endl;
        freeaddrinfo(res);
        return 2;
    }

    // Try to connect to the server:
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "ERROR: Connection failed" << std::endl;
        close(sockfd);
        freeaddrinfo(res);
        return 3;
    }

    freeaddrinfo(res);

    // open file
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: File not found" << std::endl;
        close(sockfd);
        return 4;
    }
    
    char buffer[1024];
    while (!file.eof()) {
        // read file
        file.read(buffer, sizeof(buffer));
        size_t bytesRead = file.gcount();

        // send file contents to server
        if (send(sockfd, buffer, bytesRead, 0) < 0) {
            std::cerr << "ERROR: Failed to send file data" << std::endl;
            file.close();
            close(sockfd);
            return 6;
        }
    }

    std::cout << "File transfer complete." << std::endl;

    file.close();
    close(sockfd);
    return 0;
}
