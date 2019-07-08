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

        RSAKey(const RSAKey& old);              /* never copy nor assign */
        RSAKey& operator=(const RSAKey& other);
    public:
        RSAKey();
        ~RSAKey();

        void fromUserName(string);
        void fromCertificate(Certificate&);
        EVP_PKEY* getPKEY();
};




#endif
