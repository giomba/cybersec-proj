#ifndef CIPHER_H
#define CIPHER_H

#include <iostream>
using namespace std;

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include "connection.h"

struct Rocket {
    uint32_t length;
    uint32_t sequence_number;
    uint32_t hmac;  /* TODO -- choose a proper size */
};

struct SpaceCraft {
    uint32_t hmac;  /* TODO -- choose a proper size */
    uint32_t sequence_number;
};

class Crypto {
private:
    EVP_CIPHER_CTX* ctx_e;    /* context will be the same for all session */
    EVP_CIPHER_CTX* ctx_d;
    uint32_t sequence_number_i = 0;
    uint32_t sequence_number_o = 0;
public:
	Crypto(const unsigned char* key_e, const unsigned char* key_d, const unsigned char* iv);
    ~Crypto();
	int encrypt(char* d_buffer, const char* s_buffer, int size);
	int decrypt(char* d_buffer, const char* s_buffer, int size);

    int send(Connection* connection, const char* s_buffer, int size);
    int recv(Connection* connection, char* d_buffer, int size);

};

#endif
