#ifndef NONCE_H
#define NONCE_H

#include <openssl/rand.h>
#include <string>
#include <valgrind/memcheck.h>

using namespace std;

#include "exception.h"

class Nonce {
    private:
        uint32_t nonce;
    public:
        Nonce();
        Nonce(string);
        string str();
};

#endif