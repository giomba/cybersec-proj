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
#include "../common/certmanager.h"
#include "../common/crypto.h"
#include "../common/debug.h"
#include "../common/exception.h"
#include "../common/key.h"
#include "../common/nonce.h"
#include "../common/protocol.h"
#include "../common/rsacrypto.h"

using namespace std;

class Client {
    private:
        Connection* connection;
        CertManager* cm;
        Crypto* crypto;
        X509* client_certificate;

        istringstream is;
        ostringstream os;

        vector<Key> handshake(void);

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
        Client(Connection*, CertManager*);
        ~Client();
        void execute(void);
};

#endif
