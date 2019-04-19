#ifndef CLIENT_H
#define CLIENT_H

#include <dirent.h>

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "../common/connection.h"
#include "../common/debug.h"
#include "../common/exception.h"
#include "../common/protocol.h"

using namespace std;

class Client {
    private:
        Connection* connection;
        istringstream is;
        ostringstream os;

        void recvCmd();
        int  recvBodyFragment(char *, int len);
        void sendCmd();

        void cmd_allo(void);
        void cmd_dele(void);
        void cmd_list(void);
        void cmd_quit(void);
        void cmd_retr(void);
        void cmd_stor(void);
        void cmd_unknown(void);

    public:
        Client(Connection* connection);
        bool execute(void);
};

#endif
