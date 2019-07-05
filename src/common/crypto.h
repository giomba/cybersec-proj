#ifndef CIPHER_H
#define CIPHER_H

#include <iostream>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/hmac.h>

#include "connection.h"
#include "protocol.h"
#include "rocket.h"
#include "spacecraft.h"

class Crypto {
private:
    EVP_CIPHER_CTX* ctx_e;    /* context will be the same for all session */
    EVP_CIPHER_CTX* ctx_d;
    const string auth_key;
    uint32_t sequence_number_i = 0;
    uint32_t sequence_number_o = 0;

    int encrypt(char*, const char*, int);
	int decrypt(char*, const char*, int);

public:
	Crypto(const string, const string, const string);
    ~Crypto();

    int send(Connection*, const char*, int);
    int recv(Connection*, char*, int);
};

#endif
