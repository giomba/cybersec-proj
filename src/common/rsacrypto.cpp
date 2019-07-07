#include "rsacrypto.h"

RSACrypto::RSACrypto(Certificate& certificate, RSAKey& privkey) : certificate(certificate), privkey(privkey){}

string RSACrypto::sign(string& buffer) {
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) throw ExSignature("RSACrypto::sign(): cannot create context");

    unsigned char* signature = new unsigned char[EVP_PKEY_size(privkey.getPKEY())];
    int signatureLen = 0;

    bool pass = EVP_SignInit(ctx, EVP_sha256())
                && EVP_SignUpdate(ctx, (unsigned char*)buffer.data(), buffer.size())
                && EVP_SignFinal(ctx, signature, (unsigned int*)&signatureLen, privkey.getPKEY());

    EVP_MD_CTX_free(ctx);

    string signatureString((char*)signature, signatureLen);
    delete[] signature;

    if (!pass) throw ExSignature("RSACrypto::sign(): EVP_Sign()");

    return signatureString;
}

void RSACrypto::verify(string& buffer, string& signature, RSAKey& pubkey) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) throw ExSignature("RSACrypto::verify(): cannot create context");

    bool pass = EVP_VerifyInit(ctx, EVP_sha256())
                && EVP_VerifyUpdate(ctx, buffer.data(), buffer.size())
                && EVP_VerifyFinal(ctx, (const unsigned char*)signature.data(), signature.size(), pubkey.getPKEY());

    EVP_MD_CTX_free(ctx);

    if (!pass) throw ExSignature("RSACrypto::verify(): EVP_Verify()");

    return;
}

RSASeal RSACrypto::encrypt(string& src, RSAKey& pubkey) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw ExCrypto("RSACrypto::encrypt(): cannot create context");

    EVP_PKEY* pubkey_pointer = pubkey.getPKEY();

    unsigned char* ek = new unsigned char[EVP_PKEY_size(pubkey_pointer)];
    int ekl = 0;

    unsigned char iv[AES128_KEY_LEN];

    unsigned char* out = new unsigned char[src.size() + AES128_BLOCK_LEN];
    int outl = 0;
    int cipherlen = 0;

    string outs, eks, ivs;

    bool pass = true;

    if (EVP_SealInit(ctx, EVP_aes_128_cfb8(), &ek, &ekl, iv, &pubkey_pointer, 1) != 1) {
        openssl_perror();
        pass = false;
    }
    eks.assign((char*)ek, ekl);
    ivs.assign((char*)iv, AES128_KEY_LEN);

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

    RSASeal seal;
    seal.fromEKPayloadIV(eks, outs, ivs);

    delete[] ek;
    delete[] out;
    EVP_CIPHER_CTX_free(ctx);

    if (! pass) throw ExCryptoComputation("RSACrypto::encrypt(): EVP_Seal()");

    return seal;
}

string RSACrypto::decrypt(RSASeal& seal) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) throw ExCrypto("RSACrypto::decrypt(): cannot create context");

    string eks = seal.getEK();
    int ekl = eks.size();
    unsigned char* ek = new unsigned char[ekl];
    memcpy(ek, eks.data(), ekl);

    string ps = seal.getPayload();
    int cipherlen = ps.size();
    unsigned char* ciphertext = new unsigned char[cipherlen];
    memcpy(ciphertext, ps.data(), cipherlen);

    string ivs = seal.getIV();
    int ivl = ivs.size();
    unsigned char* iv = new unsigned char[ivl];
    memcpy(iv, ivs.data(), ivl);

    unsigned char* plaintext = new unsigned char[cipherlen];
    memset(plaintext, 0, cipherlen);

    int outlen = 0, plainlen = 0;

    bool pass = true;

    if (EVP_OpenInit(ctx, EVP_aes_128_cfb8(), ek, ekl, iv, privkey.getPKEY()) == 0) {
        openssl_perror();
        pass = false;
    }
    if (EVP_OpenUpdate(ctx, plaintext, &outlen, ciphertext, cipherlen) != 1) {
        openssl_perror();
        pass = false;
    }
    plainlen = outlen;
    if (EVP_OpenFinal(ctx, plaintext + plainlen, &outlen) != 1) {
        openssl_perror();
        pass = false;
    }
    plainlen += outlen;

    string ret;
    ret.assign((char*)plaintext, plainlen);

    EVP_CIPHER_CTX_free(ctx);
    delete[] plaintext;
    delete[] ciphertext;
    delete[] ek;
    delete[] iv;

    if (! pass) throw ExCryptoComputation("RSACrypto::decrypt(): EVP_Open()");

    return ret;
}