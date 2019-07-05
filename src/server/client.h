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
#include "../common/crypto.h"
#include "../common/signer.h"
#include "../common/debug.h"
#include "../common/exception.h"
#include "../common/protocol.h"

using namespace std;

class Client {
    private:
        Connection* connection;
        CertManager* cm;
        Crypto* crypto;
        Signer* signer;
        X509* client_certificate;

        istringstream is;
        ostringstream os;

        //int handshake(unsigned char*&, unsigned char*&, unsigned char*&); // TODO
        int handshake(void);
/*         int receiveM1(X509*&);
        int sendM2(X509*, unsigned char*&, unsigned char*&, unsigned char*&);
        int receiveM3(void); */

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
        bool execute(void);
};

#endif
