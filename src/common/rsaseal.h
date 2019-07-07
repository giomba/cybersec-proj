#ifndef RSA_SEAL_H
#define RSA_SEAL_H

#include <arpa/inet.h>
#include <string>
using namespace std;

class RSASeal {
    private:
        string ek;
        string payload;
    public:
        RSASeal(string ek, string payload);
        RSASeal(string buffer);
        string str(void);
};

#endif