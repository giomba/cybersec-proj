#ifndef CERTMANAGER_H
#define CERTMANAGER_H

#include <iostream>
#include <vector>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <vector>

#include "certificate.h"
#include "debug.h"
#include "exception.h"

const string CERT_PATH = "cert/";

class CertManager {
    private:
	    Certificate cert;
        X509_STORE* store;
    public:
        CertManager(string cert_name = "server");
        ~CertManager();
        Certificate& getCert();
        int verifyCert(Certificate&, const vector<string>&);
};

#endif
