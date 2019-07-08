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
#include <fstream>
#include <vector>

#include "../common/debug.h"
#include "../common/connection.h"
#include "../common/exception.h"
#include "../common/certmanager.h"

using namespace std;

extern const string nameList;
extern const int backlog;

class Server {
    private:
        int sd;
        struct sockaddr_in6 addr;
        CertManager *cm;
        vector<string> authClientList;
    public:
        Server(const char* address, uint16_t port);
        Connection* accept(void);
        ~Server();
        CertManager* getCertManager();
};

#endif
