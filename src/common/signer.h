#ifndef SIGNER_H
#define SIGNER_H

#include <iostream>

include <openssl/pem.h>
/*#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
*/
#include "debug.h"
#include "exception.h"

class Signer {
    private:
        /* TODO -- what does it need? */
         EVP_PKEY* privkey;
    public:
        Signer(string username = "server");
        ~Signer();

        int sign(unsigned char*, int, unsigned char*, int*&, EVP_PKEY*);
        int verify(char*, int, unsigned char*, int, EVP_PKEY*);
};

#endif
