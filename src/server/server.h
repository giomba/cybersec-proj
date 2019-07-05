#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <errno.h>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "../common/debug.h"
#include "../common/connection.h"
#include "../common/exception.h"
#include "../common/certmanager.h"

using namespace std;

class Server {
    private:
        socklen_t sizeof_addr;
        int sd;
        struct sockaddr_in6 addr;
        vector<string> clientList;

        CertManager *cm;
    public:
        Server(const char* address, uint16_t port);
        Connection* accept(void);
        ~Server();
        CertManager* getCertManager();
};

#endif
