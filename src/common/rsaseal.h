#ifndef RSA_SEAL_H
#define RSA_SEAL_H

#include <arpa/inet.h>
#include <cassert>
#include <string>
using namespace std;

class RSASeal {
    private:
        string ek;
        string payload;
    public:
        //RSASeal(void);
        void fromEKPayload(string& ek, string& payload);
        void fromString(string& buffer);
        string str(void);
};

#endif