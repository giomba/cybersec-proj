#include "connection.h"

Connection::Connection(int sd, struct sockaddr_in6 peer) {
    debug(INFO, "[I] new connection " << this << endl);
    this->sd = sd;
    this->peer = peer;
}

Connection::Connection(const char* hostname, uint16_t port) {
    // do everything is needed to connect
    if ((this->sd = socket(AF_INET6, SOCK_STREAM, 0)) == -1) {
        throw ExSocket("can not create socket() for new Connection()", errno);
    }
    memset(&peer, 0, sizeof(peer));
    peer.sin6_family = AF_INET6;
    peer.sin6_port = htons(port);
    inet_pton(AF_INET6, hostname, &peer.sin6_addr);

    if (::connect(this->sd, (struct sockaddr*)&peer, sizeof(peer)) == -1) {
        throw ExConnect("can not connect() for new Connection()", errno);
    };
}

int Connection::getSocket(){
    return this->sd;
}

int Connection::send(string& buffer) {
    uint32_t size = buffer.size();
    size = htonl(size);
    int ret1, ret2;
    ret1 = this->send((const char*)&size, sizeof(size));
    ret2 = this->send(buffer.data(), buffer.size());
    if (ret1 >= 0 && ret2 >= 0) return ret1 + ret2;
    else return -1;
}

int Connection::recv(string& buffer) {
    uint32_t size;
    int ret1, ret2;

    ret1 = this->recv((char*)&size, sizeof(size));
    size = ntohl(size);

    if (size > BUFFER_SIZE) throw ExTooBig("string too big");

    char* tmp_buffer = new char[size];
    ret2 = this->recv(tmp_buffer, size);

    buffer.assign(tmp_buffer, size);

    delete[] tmp_buffer;

    if (ret1 >= 0 && ret2 >= 0) return ret1 + ret2;
    else return -1;
}

int Connection::send(const char* buffer, int len) {
    debug(DEBUG, "[D] === Connection::send()" << endl);
    hexdump(DEBUG, buffer, len);

    int ret = ::send(this->sd, (void*)buffer, len, MSG_NOSIGNAL);
    if (ret <= 0) {
        throw ExSend("can not send()");
    }
    return ret;
}

int Connection::recv(char* buffer, int len) {
    int ret = ::recv(this->sd, (void*)buffer, len, MSG_WAITALL);
    if (ret <= 0) {
        throw ExRecv("can not recv()");
    }

    return ret;
}

Connection::~Connection() {
    debug(INFO, "[I][" << this->sd << "] destroyng connection " << this << endl);
    if (this->sd != 0) close(this->sd);
    // close connections and destroy created sockets, if necessary
    // (eg. if socket was created using this->connect() )
}

