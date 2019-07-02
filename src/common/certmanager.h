#ifndef CERTMANAGER_H
#define CERTMANAGER_H

#include <iostream>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include "debug.h"
#include "exception.h"
#include "protocol.h"

const string CERT_PATH = "cert/";

class CertManager {
    private:
	    X509* cert;
        X509_STORE* store;
    public:
        CertManager(string);
        ~CertManager();
        X509* getCert();
        int cert_verification(X509*, string name = "");
};

#endif
