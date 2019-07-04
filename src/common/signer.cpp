#include "signer.h"

Signer::Signer(){

}

Signer::~Signer(){

}

int Signer::sign(unsigned char* msg, int msgLen, unsigned char*& signature, unsigned int*& signatureLen, EVP_PKEY* privkey){

}

int Signer::verify(X509* cert, char* msg, int msgLen, unsigned char* signature, int signatureLen){
    
}