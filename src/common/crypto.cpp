#include "crypto.h"

using namespace std;

Crypto::Crypto(const unsigned char* key_e, const unsigned char* key_d, const unsigned char * iv) {
    //create and initialize context
	ctx_e = EVP_CIPHER_CTX_new();

	//Initialize the encryption setting
	EVP_EncryptInit(ctx_e, EVP_aes_128_cfb8(), key_e, iv);

    ctx_d = EVP_CIPHER_CTX_new();
    EVP_DecryptInit(ctx_d, EVP_aes_128_cfb8(), key_d, iv);
}

int Crypto::encrypt(char* d_buffer, const char* s_buffer, int size){
	int len, r;

	//Encrypt and return the buffer
	if ((r = EVP_EncryptUpdate(ctx_e, (unsigned char*)d_buffer, &len, (const unsigned char*)s_buffer, size)) == 0) {
        // TODO -- throw some exception
    }

	return r;
}

int Crypto::decrypt(char* d_buffer, const char* s_buffer, int size){
	int len, r;

	//Encrypt and return the buffer
	if ((r = EVP_DecryptUpdate(ctx_d, (unsigned char*)d_buffer, &len, (const unsigned char*)s_buffer, size)) == 0) {
        // TODO -- throw some exception
    }

	return r;
}

Crypto::~Crypto() {
    /* TODO -- this should not be needed */
    /*
	EVP_EncryptFinal(ctx_e, ciphertext+len, &len);
	EVP_DecryptFinal(ctx, decryptedtext+len, &len);
    */

    EVP_CIPHER_CTX_free(ctx_e);
	EVP_CIPHER_CTX_free(ctx_d);
}
