#ifndef RSA_SEAL_H
#define RSA_SEAL_H

#include <arpa/inet.h>
#include <cassert>
#include <string>

#include "protocol.h"

using namespace std;

class RSASeal {
    private:
        string ek;
        string iv;
        string payload;
    public:
        //RSASeal(void);
        void fromEKPayloadIV(string& ek, string& payload, string& iv);
        void fromString(string& buffer);

        string getEK(void);
        string getPayload(void);
        string getIV(void);
        
        string str(void);
};

#endif