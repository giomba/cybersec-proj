#ifndef CERTIFICATE_H
#define CERTIFICATE_H

#include <iostream>
#include <openssl/x509.h>
#include <string>
using namespace std;

#include "debug.h"
#include "exception.h"

class Certificate {
    private:
        X509* cert;
    public:
        Certificate(void);
        Certificate(X509*);
        Certificate(string);
        ~Certificate(void);
        Certificate(const Certificate& old);

        X509* getX509(void);
        string str(void);
};

#endif