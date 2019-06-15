#ifndef CIPHER_H
#define CIPHER_H

#include <iostream>
using namespace std;

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/hmac.h>

#include "connection.h"

const int HMAC_SIZE = 32; // EVP_MD_size(EVP_sha256());

struct Rocket {
    unsigned char hmac[HMAC_SIZE];
    uint32_t length;
    uint32_t sequence_number;
};

struct SpaceCraft {
    unsigned char hmac[HMAC_SIZE];
    uint32_t sequence_number;
};

class Crypto {
private:
    EVP_CIPHER_CTX* ctx_e;    /* context will be the same for all session */
    EVP_CIPHER_CTX* ctx_d;
    unsigned char* auth_key;
    uint32_t sequence_number_i = 0;
    uint32_t sequence_number_o = 0;

    int encrypt(char* d_buffer, const char* s_buffer, int size);
	int decrypt(char* d_buffer, const char* s_buffer, int size);

    int hmac(unsigned char* digest, const Rocket& rocket);
    int hmac(unsigned char* digest, const SpaceCraft& spacecraft, const unsigned char* encrypted_payload, int size);

public:
	Crypto(const unsigned char* key_e, const unsigned char* key_d, const unsigned char* iv);
    ~Crypto();

    int send(Connection* connection, const char* s_buffer, int size);
    int recv(Connection* connection, char* d_buffer, int size);
};

#endif
