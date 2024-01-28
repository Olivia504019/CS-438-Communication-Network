#ifndef SOCKET_H_
#define SOCKET_H_

#include "packet.h"
#include <utility>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>

static struct sockaddr_in tmp;

class Socket{
private:
    int sock_fd;
public:
    Socket();
    ~Socket();
    void bind(int);
    std::pair<Packet, bool> receiveData(struct sockaddr_in& = tmp);
    void sendData(struct sockaddr_in, Packet);
    static struct sockaddr_in getSocketAddressFromHostname(char*, unsigned short int);
};

#endif  // SOCKET_H_