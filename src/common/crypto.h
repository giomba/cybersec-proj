#ifndef CIPHER_H
#define CIPHER_H

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

class Crypto {
private:
    EVP_CIPHER_CTX* ctx_e;    /* context will be the same for all session */
    EVP_CIPHER_CTX* ctx_d;
public:
	Crypto(const unsigned char* key_e, const unsigned char* key_d, const unsigned char* iv);
    ~Crypto();
	int encrypt(char* d_buffer, const char* s_buffer, int size);
	int decrypt(char* d_buffer, const char* s_buffer, int size);
};

#endif
