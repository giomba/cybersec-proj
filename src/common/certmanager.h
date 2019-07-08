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
#include "rsakey.h"

extern const string CERT_PATH;

class CertManager {
    private:
	    Certificate cert;
        RSAKey privkey;
        X509_STORE* store;
        vector<string>& authPeersList;
    public:
        CertManager(string cert_name, vector<string>& authPeersList);
        ~CertManager();
        Certificate& getCert(void);
        RSAKey& getPrivKey(void);
        void verifyCert(Certificate&);
};

#endif
