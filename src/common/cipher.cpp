#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/enc.h>

#include "cipher.h"

#typedef unsigned char

Crypto::Crypto(const char* key, const char * iv){
	this->key = key;
	this->iv = iv;
}

const char* Crypto::encrypt(const char* bufferName, size_t size){
	//allocate the memory for the ciphetext
	unsigned char* ciphertext = (unsigned char*)malloc(size+16);
	int len;

	//create and initialize context
	EVP_CIPHER_CTX *ctx;
	ctx = EVP_CIPHER_CTX_new();

	//Initialize the encryption setting
	EVP_EncryptInit(ctx, EVP_aes_128_ecb(), key, iv);

	//Encrypt and return the buffer
	EVP_EncryptUpdate(ctx, ciphertext, &len, (const unsigned char*)bufferName, size);

	EVP_EncryptFinal(ctx, ciphertext+len, &len);
		
	EVP_CIPHER_CTX_free(ctx);

	return ciphertext;
}

const char* Crypto::decrypt(const char* bufferName, size_t size){
	//allocate the memory for the ciphetext
	unsigned char* decryptedtext = (unsigned char*)malloc(size);
	int len;

	//create and initialize context
	EVP_CIPHER_CTX *ctx;
	ctx = EVP_CIPHER_CTX_new();

	//Initialize the encryption setting
	EVP_DecryptInit(ctx, EVP_aes_128_ecb(), key, iv);

	//Encrypt and return the buffer
	EVP_DecryptUpdate(ctx, decryptedtext, &len, (const unsigned char*)bufferName, size);

	EVP_DecryptFinal(ctx, decryptedtext+len, &len);
		
	EVP_CIPHER_CTX_free(ctx);

	return decryptedtext;
}
