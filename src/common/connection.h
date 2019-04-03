#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdlib>
#include <cstdint>
#include <netinet/in.h>
#include <stdio.h>


//using namespace std;

// Note: this is not the final interface!
// Note: these methods _must_ throw exceptions

class Connection {
    private:
        int sd;
        struct sockaddr_in6 peer;
    public:
        Connection(const char* hostname, uint16_t port);
        Connection(int socket);
        ~Connection();
        int connect(void);
        int send(const char* buffer, size_t len);
        int recv(char* buffer, size_t len);
};

#endif

