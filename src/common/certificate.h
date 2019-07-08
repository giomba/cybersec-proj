#ifndef CERTIFICATE_H
#define CERTIFICATE_H

#include <cassert>
#include <iostream>
#include <openssl/x509.h>
#include <string>
using namespace std;

#include "debug.h"
#include "exception.h"

class Certificate {
    private:
        X509* cert;

        Certificate(const Certificate& old);    /* never copy */
    public:
        Certificate(void);
        ~Certificate(void);

        void fromX509(X509*);
        void fromString(string);

        X509* getX509(void);
        string str(void);
};

#endif
