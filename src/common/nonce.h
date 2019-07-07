#ifndef NONCE_H
#define NONCE_H

#include <string>

#include "cybrand.h"

using namespace std;

class Nonce {
    private:
        uint32_t nonce;
    public:
        Nonce();
        Nonce(string);
        string str();
};

#endif