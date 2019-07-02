#include "server.h"

socklen_t Server::sizeof_addr = sizeof(addr);

Server::Server(const char* address, uint16_t port) {
    sd = socket(AF_INET6, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    inet_pton(AF_INET6, address, &addr.sin6_addr);

    if ( ::bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 ) {
        throw ExBind("server creation", errno);
    }
    debug(INFO, "bind() ok" << endl);

    if ( ::listen(sd, 10) != 0 ) {    // TODO: choose a proper number
        throw ExListen("listen()");
    }
    debug(INFO, "listen() ok" << endl);

    this->cm = new CertManager("server");
}

Connection* Server::accept() {
    struct sockaddr_in6 client_addr;
    int client_sd = ::accept(sd, (struct sockaddr*)&client_addr, &sizeof_addr);
    debug(INFO, "[I] remote TCP port: " << client_addr.sin6_port << endl);
    return new Connection(client_sd, client_addr, this->cm);
}

Server::~Server() {
    // please someone implement me :-(
}
