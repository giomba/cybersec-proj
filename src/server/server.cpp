#include <arpa/inet.h>
#include <errno.h>
#include <cstring>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../common/debug.h"
#include "../common/connection.h"
#include "../common/exception.h"
#include "server.h"

socklen_t Server::sizeof_addr = sizeof(addr);

Server::Server(const char* address, uint16_t port) {
    sd = socket(AF_INET6, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    inet_pton(AF_INET6, address, &addr.sin6_addr);

    if ( bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 ) {
        throw ExBind("server creation", errno);
    }
    debug(INFO, "bind() ok");

    if ( listen(sd, 10) != 0 ) {    // TODO: choose a proper number
        throw ExListen("listen()");
    }
    debug(INFO, "listen() ok");
}

Connection Server::accept() {
    struct sockaddr_in6 client_addr;
    int client_sd = ::accept(sd, (struct sockaddr*)&client_addr, &sizeof_addr);
    return Connection(client_sd, client_addr);
}

Server::~Server() {
    // please someone implement me :-(
}
