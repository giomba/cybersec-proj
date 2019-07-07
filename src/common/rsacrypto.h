#ifndef RSA_CRYPTO_H
#define RSA_CRYPTO_H

#include <string>
using namespace std;

#include "certificate.h"
#include "protocol.h"
#include "rsakey.h"
#include "rsaseal.h"

class RSACrypto {
    private:
        Certificate& certificate;   /* public certificate */
        RSAKey& privkey;            /* corresponding private key */
    public:
        RSACrypto(Certificate& certificate, RSAKey& privkey);

        string sign(string& buffer);
        void verify(string& buffer, string& signature, RSAKey& pubkey);

        RSASeal encrypt(string& src, RSAKey& pubkey);
        string decrypt(RSASeal& src);
};

#endif