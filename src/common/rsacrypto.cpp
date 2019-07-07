#include "rsacrypto.h"

RSACrypto::RSACrypto(Certificate& certificate, RSAKey& privkey) : certificate(certificate), privkey(privkey){
    ;
}

void RSACrypto::sign(string& buffer, string& signature) {
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    signature.reserve(EVP_PKEY_size(privkey.getPKEY()));
    int signatureLen = 0;
	bool pass = true;

    pass = EVP_SignInit(ctx, EVP_sha256())
           && EVP_SignUpdate(ctx, (unsigned char*)buffer.c_str(), buffer.size())
           && EVP_SignFinal(ctx, (unsigned char*)signature.c_str(), (unsigned int*)&signatureLen, privkey.getPKEY());
    if (!pass) throw ExSignature("EVP_Sign()");

    signature.resize(signatureLen);

    EVP_MD_CTX_free(ctx);
}

int RSACrypto::verify(string& buffer, string& signature, EVP_PKEY* pubkey) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();

    EVP_VerifyInit(ctx, EVP_sha256());
    EVP_VerifyUpdate(ctx, buffer.c_str(), buffer.size());
    if ( EVP_VerifyFinal(ctx, (const unsigned char*)signature.c_str(), signature.size(), pubkey) != 1) {
        throw ExSignature("EVP_Verify()");
    }

    EVP_MD_CTX_free(ctx);

    return 0;
}

RSASeal RSACrypto::encrypt(string& src, RSAKey& pubkey) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_PKEY* pubkey_pointer = pubkey.getPKEY();

    unsigned char iv[AES128_KEY_LEN];
    memset(iv, 0, AES128_KEY_LEN);

    unsigned char* ek = new unsigned char[EVP_PKEY_size(pubkey_pointer)];
    int ekl = 0;

    unsigned char* out = new unsigned char[src.size() + AES128_BLOCK_LEN];
    int outl = 0;
    int cipherlen = 0;

    string outs, eks;

    int pass = true;

    if (EVP_SealInit(ctx, EVP_aes_128_cbc(), &ek, &ekl, iv, &pubkey_pointer, 1) != 1) {
        openssl_perror();
        pass = false;
    }
    eks.assign((char*)ek, ekl);
    if (EVP_SealUpdate(ctx, out, &outl, (unsigned char*)src.data(), src.size()) != 1) {
        openssl_perror();
        pass = false;
    }
    cipherlen = outl;
    if (EVP_SealFinal(ctx, out + cipherlen, &outl) != 1) {
        openssl_perror();
        pass = false;
    }
    cipherlen += outl;
    outs.assign((char*)out, cipherlen);

    RSASeal seal(eks, outs);

    delete[] ek;
    delete[] out;
    EVP_CIPHER_CTX_free(ctx);

    if (! pass) throw ExCryptoComputation("can not RSA Encrypt Seal()");

    return seal;
}

string RSACrypto::decrypt(string& src) {
    assert(false);
}