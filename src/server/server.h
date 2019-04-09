#ifndef SERVER_H
#define SERVER_H

class Server {
    private:
        static socklen_t sizeof_addr;

        int sd;
        struct sockaddr_in6 addr;
    public:
        Server(const char* address, uint16_t port);
        Connection* accept(void);
        ~Server();
};

#endif
