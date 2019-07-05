#ifndef CERTIFICATE_H
#define CERTIFICATE_H

#include <openssl/x509.h>

class Certificate {
    private:
        X509* cert;
    public:
        Certificate(void);
        Certificate(X509*);
        ~Certificate(void);

        X509* getX509(void);
};

#endif