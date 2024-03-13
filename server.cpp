#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>


void handleConnection(int clientSocketfd, const std::string& dir, int connectionID) {
    // create output file path and open it
    std::string filePath = dir + "/" + std::to_string(connectionID) + ".file";
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "ERROR: Could not open file for writing." << std::endl;
        return;
    }

    char buffer[1024];
    ssize_t bytesRead;

    // set up timeout
    struct timeval tv;
    tv.tv_sec = 10; // 10 second timeout
    tv.tv_usec = 0;
    setsockopt(clientSocketfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    while (true) {
        // read client file content
        bytesRead = recv(clientSocketfd, buffer, sizeof(buffer), 0);
        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cerr << "ERROR: Timeout occurred. No data recieved." << std::endl;
                outFile.seekp(0);
                outFile.clear();
                outFile << "ERROR";
                break;
            } else {
                std::cerr << "ERROR: " << strerror(errno) << std::endl;
                break;
            }
        } else if (bytesRead == 0) {
            // client closed connection
            break;
        } else {
            // write content to new file
            outFile.write(buffer, bytesRead);
        }
    }

    outFile.close();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <PORT> <FILE-DIR>" << std::endl;
        return 1;
    }

    int port = atoi(argv[1]);
    if (port <= 1023 || port > 65535) {
        std::cerr << "ERROR: Invalid port number" << std::endl;
        return 2;
    }

    std::string fileDir = argv[2];

    // create a socket using TCP IP
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "ERROR: " << strerror(errno) << std::endl;
        return 3;
    }

    // allow others to reuse the address
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        std::cerr << "ERROR: " << strerror(errno) << std::endl;
        close(sockfd);
        return 4;
    }

    // bind address to socket
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "ERROR: " << strerror(errno) << std::endl;
        close(sockfd);
        return 5;
    }

    // set socket to listen status
    if (listen(sockfd, 1) == -1) {
        std::cerr << "ERROR: " << strerror(errno) << std::endl;
        close(sockfd);
        return 6;
    }

    // connection counter
    int connectionID = 0;
    while (true) {
        // accept a new connection from a client
        struct sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        int clientSocketfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocketfd == -1) {
            if (errno == EINTR) {
                break;
            }
            std::cerr << "ERROR: " << strerror(errno) << std::endl;
            continue;
        }

        connectionID += 1;
        handleConnection(clientSocketfd, fileDir, connectionID);
        close(clientSocketfd);
    }

    close(sockfd);
    return 0;
}
