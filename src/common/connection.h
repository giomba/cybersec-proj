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
        ssize_t send(const char* buffer, size_t len);
        ssize_t recv(char* buffer, size_t len);
};

#endif

