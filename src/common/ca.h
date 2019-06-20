#ifndef CERTIFICATIONAUTHORITY_H
#define CERTIFICATIONAUTHORITY_H

#include <iostream>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include "debug.h"
#include "exception.h"
#include "protocol.h"

const string CERT_PATH = "cert/";

class CertificationAuthority {
    private:
	X509* cert;
        X509_STORE* store;
    public:
        CertificationAuthority(string);
        ~CertificationAuthority();
        int cert_verification(X509*, string);
};

#endif
