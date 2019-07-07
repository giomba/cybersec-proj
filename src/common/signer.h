#ifndef SIGNER_H
#define SIGNER_H

#include <iostream>

#include <openssl/pem.h>
//#include <openssl/x509.h>
//#include <openssl/x509_vfy.h>

#include "debug.h"
#include "exception.h"

//const string KEY_PATH = "cert/";
class Signer {
    private:
        /* TODO -- what does it need? */
         EVP_PKEY* privkey;
    public:
        Signer(string username = "server");
        ~Signer();

        int sign(unsigned char*, int, unsigned char*, unsigned int*);
        int verify(char*, int, unsigned char*, int, EVP_PKEY*);
        EVP_PKEY* getPrivKey();
};

#endif
