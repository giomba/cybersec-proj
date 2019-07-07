#include "spacecraft.h"

SpaceCraft::SpaceCraft(){}

SpaceCraft::SpaceCraft(uint32_t sequence_number){
    this->sequence_number = sequence_number;
}

void SpaceCraft::htonl(){
    this->sequence_number = ::htonl(sequence_number);
}

void SpaceCraft::ntohl(){
    this->sequence_number = ::ntohl(sequence_number);
}

void SpaceCraft::computehmac(Key& auth_key, const string& payload){
    unsigned int len;

    HMAC_CTX* ctx = HMAC_CTX_new();
    if (!ctx) throw ExCryptoComputation("SpaceCraft::hmac() out of memory");

    bool pass =
        HMAC_Init_ex(ctx, (const char*)auth_key.str().c_str(), HMAC_LEN, EVP_sha256(), NULL)
        && HMAC_Update(ctx, (const unsigned char*)&this->sequence_number, sizeof(this->sequence_number))
        && HMAC_Update(ctx, (const unsigned char*)payload.c_str(), payload.size())
        && HMAC_Final(ctx, this->hmac, &len);

    HMAC_CTX_free(ctx);

    if (!pass) throw ExCryptoComputation("Crypto::hmac() can not compute");

    return;
}

void SpaceCraft::verify(Key& auth_key, uint32_t expected_seq_num, const string& payload){
    string received_hmac((const char*)&this->hmac, HMAC_LEN);
    this->computehmac(auth_key, payload);

    if (CRYPTO_memcmp((const char*)&this->hmac, (const char*)received_hmac.c_str(), HMAC_LEN) != 0)
        throw ExBadHMAC("SpaceCraft::verify(): bad hmac");

    if (this->sequence_number != expected_seq_num)
        throw ExBadSeqNum("SpaceCraft::verify(): invalid seqence number");

    return;
}

