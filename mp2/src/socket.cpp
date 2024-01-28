#include "socket.h"
#include <sys/socket.h>
#include <utility>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <bitset>
#include <stdlib.h>

Socket::Socket() {
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        perror("createSocket: socket create failure");
        exit(1);
    }
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1){
        perror("createSocket: socket set receive timeout failure");
        exit(1);
    }
}

Socket::~Socket() {
    ::close(sock_fd);
}

void Socket::bind(int port){
    struct sockaddr_in address;

    memset((char *) &address, 0, sizeof (address));

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (::bind(sock_fd, (struct sockaddr*) &address, sizeof (address)) == -1){
        perror("bind failure");
        exit(1);
    }
}

std::pair<Packet, bool> Socket::receiveData(struct sockaddr_in &address) {
    socklen_t sin_size = sizeof(address);
    Packet packet;
	if (recvfrom(sock_fd, &packet, sizeof(packet), 0, (struct sockaddr*)&address, &sin_size) == -1){
        if (errno == EWOULDBLOCK || errno == EAGAIN) // Timeout
            return std::make_pair(packet, true);
        else{
            perror("receive data fails");
            exit(1);
        }
    }
    return std::make_pair(packet, false);
}

void Socket::sendData(struct sockaddr_in address, Packet packet){
    // if (rand() % 100 >= 95) return;
    socklen_t sin_size = sizeof(address);
    if (sendto(sock_fd, (char*) &packet, sizeof(packet), 0, (struct sockaddr*)&address, sin_size) == -1) {
        perror("send data fails");
        exit(1);
    }
}

struct sockaddr_in Socket::getSocketAddressFromHostname(char* host_name, unsigned short int host_udp_port){
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(host_udp_port);
    if (inet_aton(host_name, &address.sin_addr) == 0){
        perror("get address fails");
        exit(1);
    }
    return address;
}
