#ifndef SIGNER_H
#define SIGNER_H

#include <iostream>
/*
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
*/
#include "debug.h"
#include "exception.h"

class Signer {
    private:
        /* TODO -- what does it need? */
    public:
        Signer();
        ~Signer();

        int sign(unsigned char*, int, unsigned char*, int*&, EVP_PKEY*);
        int verify(X509*, char*, int, unsigned char*, int);
};

#endif
