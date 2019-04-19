#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdlib>
#include <cstdint>
#include <netinet/in.h>
#include <stdio.h>

class Connection {
    private:
        int sd;
        struct sockaddr_in6 peer;
    public:
        Connection(const char* hostname, uint16_t port);
        Connection(int sd, sockaddr_in6 peer);
        ~Connection();
        int send(const char* buffer, int len);
        int recv(char* buffer, int len);
};

#endif

