#include "rocket.h"

Rocket::Rocket(){}

Rocket::Rocket(uint32_t size, uint32_t sequence_number){
    this->payload_size = size;
    this->sequence_number = sequence_number;
}

void Rocket::htonl(){
    this->payload_size = ::htonl(this->payload_size);
    this->sequence_number = ::htonl(this->sequence_number);
}

void Rocket::ntohl(){
    this->payload_size = ::ntohl(this->payload_size);
    this->sequence_number = ::ntohl(this->sequence_number);
}

int Rocket::getPayloadSize(){
    return this->payload_size;
}

void Rocket::computehmac(Key& auth_key){
    unsigned int len;

    HMAC_CTX* ctx = HMAC_CTX_new();
    if (!ctx) throw ExCryptoComputation("Rocket::hmac(): out of memory");

    bool pass =
        HMAC_Init_ex(ctx, (const char*)auth_key.str().c_str(), HMAC_LEN, EVP_sha256(), NULL)
        && HMAC_Update(ctx, (const unsigned char*)&this->payload_size, sizeof(this->payload_size))
        && HMAC_Update(ctx, (const unsigned char*)&this->sequence_number, sizeof(this->sequence_number)) 
        && HMAC_Final(ctx, (unsigned char*)&this->hmac, &len);

    HMAC_CTX_free(ctx);

    if (!pass) throw ExCryptoComputation("Rocket::hmac(): cannot compute");

    return;
}

void Rocket::verify(Key& auth_key, uint32_t expected_seq_num){
    string received_hmac((const char*)&this->hmac, HMAC_LEN);
    this->computehmac(auth_key);

    if (CRYPTO_memcmp((const char*)&this->hmac, (const char*)received_hmac.c_str(), HMAC_LEN) != 0)
        throw ExBadHMAC("Rocket::verify(): bad hmac");

    if (this->payload_size > BUFFER_SIZE)
        throw ExBadProtocol("Rocket::verify(): length too long");

    if (this->sequence_number != expected_seq_num)
        throw ExBadSeqNum("Rocket::verify(): invalid seqence number");

    return;
}