#include "signer.h"

using namespace std;

Signer::Signer(string username){
	// load the private key
    
    FILE *file = fopen((KEY_PATH + username + "_key.pem").c_str(), "r");
    if (!file) { debug(FATAL, "cannot open " + username + " private key" << endl); throw ExCertificate(); }
	this->privkey = PEM_read_PrivateKey(file, NULL, NULL, NULL);
	fclose(file);
	if (!this->privkey) { debug(FATAL, "cannot read my private key" << endl); throw ExCertificate(); }
}

Signer::~Signer(){
	EVP_PKEY_free(this->privkey);
}

EVP_PKEY* Signer::getPrivKey(){
	return this->privkey;
	}
	
int Signer::sign(unsigned char* msg, int msgLen, unsigned char* signature, unsigned int* signatureLen){
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	int ret = 0;
	bool pass = true;
	
    pass = EVP_SignInit(ctx, EVP_sha256())
           && EVP_SignUpdate(ctx, (unsigned char*)&msg, sizeof(msgLen))
           && EVP_SignFinal(ctx, (unsigned char*)signature, (unsigned int*)&signatureLen, this->privkey);
    if (!pass){
        debug(ERROR, "[E] EVP_Sign()" << endl);
        ret = -1;
    }
    EVP_MD_CTX_free(ctx);
	return ret;
}

int Signer::verify( char* msg, int msgLen, unsigned char* signature, int signatureLen, EVP_PKEY* pubkey){
    /* check signature */
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_VerifyInit(ctx, EVP_sha256());
    EVP_VerifyUpdate(ctx, msg, msgLen);
    int ret = EVP_VerifyFinal(ctx, signature, signatureLen, pubkey);
    debug(ERROR, "[E] EVP wrong signature" << endl);
    EVP_MD_CTX_free(ctx);

	return (ret != 1) ? -1 : 0;
}
