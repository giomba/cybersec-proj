#ifndef CONNECTION_H
#define CONNECTION_H

#include <arpa/inet.h>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <netinet/in.h>
#include <iostream>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "exception.h"
#include "protocol.h"

class Connection {
    private:
        int sd;
        struct sockaddr_in6 peer;
    public:
        Connection(const char* hostname, uint16_t port);
        Connection(int sd, sockaddr_in6 peer);
        ~Connection();
        int getSocket();
        int send(const char* buffer, int len);
        int recv(char* buffer, int len);
        int send(string&);
        int recv(string&);
};

#endif

