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
#include "certmanager.h"

class Connection {
    private:
        int sd;
        struct sockaddr_in6 peer;
        CertManager *cm;
    public:
        Connection(const char* hostname, uint16_t port, CertManager* cm);
        Connection(int sd, sockaddr_in6 peer, CertManager* cm);
        ~Connection();
        int handshakeServer(void);
        int handshakeClient(void);
        int send(const char* buffer, int len);
        int recv(char* buffer, int len);
};

#endif

