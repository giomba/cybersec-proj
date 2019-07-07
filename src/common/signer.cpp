/*
#include "signer.h"

using namespace std;

Signer::Signer(string username){

}

Signer::~Signer(){
	EVP_PKEY_free(this->privkey);
}

EVP_PKEY* Signer::getPrivKey(){
	return this->privkey;
	}

int Signer::sign(unsigned char* msg, int msgLen, unsigned char* signature, unsigned int* signatureLen){

}

int Signer::verify( char* msg, int msgLen, unsigned char* signature, int signatureLen, EVP_PKEY* pubkey){
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_VerifyInit(ctx, EVP_sha256());
    EVP_VerifyUpdate(ctx, msg, msgLen);
    int ret = EVP_VerifyFinal(ctx, signature, signatureLen, pubkey);
    debug(ERROR, "[E] EVP wrong signature" << endl);
    EVP_MD_CTX_free(ctx);

	return (ret != 1) ? -1 : 0;
}
 */