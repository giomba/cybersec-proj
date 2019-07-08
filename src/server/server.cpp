#include "server.h"
const int backlog = 10;
const string nameList = "conf/authorized_clients.txt";

Server::Server(const char* address, uint16_t port) {
    sd = socket(AF_INET6, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    inet_pton(AF_INET6, address, &addr.sin6_addr);

    if ( ::bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 ) {
        throw ExBind("server creation", errno);
    }
    debug(INFO, "[I] bind() ok" << endl);

    if ( ::listen(sd, backlog) != 0 ) {
        throw ExListen("listen()");
    }
    debug(INFO, "[I] listen() ok" << endl);

    //intializing the list of clients
	string clientName;
    ifstream in(nameList);
    while(getline(in, clientName)) {
        authClientList.push_back(clientName);
    }
    in.close();
    this->cm = new CertManager("server", authClientList);
}

Connection* Server::accept() {
    struct sockaddr_in6 client_addr;
    int addrlen = sizeof(client_addr);
    int client_sd = ::accept(sd, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen);
    debug(INFO, "[I] remote TCP port: " << client_addr.sin6_port << endl);
    return new Connection(client_sd, client_addr);
}

Server::~Server() {
    delete this->cm;
}

CertManager* Server::getCertManager(){
    return this->cm;
}
