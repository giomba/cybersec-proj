#ifndef CLIENT_H
#define CLIENT_H

#include "../common/connection.h"

class Client {
    private:
        Connection connection;
    public:
        Client(Connection connection);
        bool execute(void);
};

#endif
