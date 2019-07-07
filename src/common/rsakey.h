#ifndef RSA_KEY_H
#define RSA_KEY_H

#include <cassert>
#include <openssl/pem.h>
#include <string>
using namespace std;

#include "certificate.h"
#include "exception.h"
#include "rsakey.h"

extern const string KEY_PATH;

class RSAKey {
    private:
        EVP_PKEY* key;
    public:
        RSAKey();
        RSAKey(const RSAKey& old);
        RSAKey& operator=(const RSAKey& other);
        ~RSAKey();

        void fromUserName(string);
        void fromCertificate(Certificate&);
        void fromString(string);
        EVP_PKEY* getPKEY();
};




#endif
