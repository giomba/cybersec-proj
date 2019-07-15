#include "crypto.h"

using namespace std;

Crypto::Crypto(Key& session_key, Key& auth_key, Key& iv) : auth_key(auth_key) {
    //create and initialize context for encryption and decryption
	ctx_e = EVP_CIPHER_CTX_new();
	EVP_EncryptInit(ctx_e, EVP_aes_128_cfb8(), (const unsigned char*)session_key.str().c_str(), (const unsigned char*)iv.str().data());

    ctx_d = EVP_CIPHER_CTX_new();
    EVP_DecryptInit(ctx_d, EVP_aes_128_cfb8(), (const unsigned char*)session_key.str().c_str(), (const unsigned char*)iv.str().data());

    debug(DEBUG, "[D] session_key + auth_key + iv" << endl);
    hexdump(DEBUG, (const char*)session_key.str().data(), AES128_KEY_LEN);
    hexdump(DEBUG, (const char*)auth_key.str().data(), HMAC_LEN);
    hexdump(DEBUG, (const char*)iv.str().data(), AES128_KEY_LEN);

    sequence_number_i = sequence_number_o = 0;
}

int Crypto::encrypt(char* dest, const char* source, int size){
	int len, r;

    /* Encrypt and update the buffer */
	if ((r = EVP_EncryptUpdate(ctx_e, (unsigned char*)dest, &len, (const unsigned char*)source, size)) == 0) {
        throw ExCryptoComputation("can not EVP_EncryptUpdate");
    }

	return r;
}

int Crypto::decrypt(char* dest, const char* source, int size){
	int len, r;

	/* Decrypt and update the buffer */
	if ((r = EVP_DecryptUpdate(ctx_d, (unsigned char*)dest, &len, (const unsigned char*)source, size)) == 0) {
        throw ExCryptoComputation("can not EVP_DecryptUpdate");
    }

	return r;
}

int Crypto::send(Connection* connection, const char* plaintext, int size) {
    char encrypted_payload[BUFFER_SIZE];
    encrypt(encrypted_payload, plaintext, size);    /* Payload encryption */

    /* Rocket preparation */
    Rocket rocket(size, sequence_number_o++);
    rocket.computehmac(auth_key); /* computes Rocket hmac */
    rocket.htonl();

    /* Spacecraft preparation */
    SpaceCraft spacecraft(sequence_number_o++);
    const string enc_payload_str(encrypted_payload, size);
    spacecraft.computehmac(auth_key, enc_payload_str); /* computes SpaceCraft hmac */
    spacecraft.htonl();

    /* Debug output */
    debug(DEBUG, "[D] === Crypto::send() ===" << endl);
    debug(DEBUG, "[D] PlainText:  " << endl); hexdump(DEBUG, plaintext, size);
    debug(DEBUG, "[D] Rocket:     " << endl); hexdump(DEBUG, (const char*)&rocket, sizeof(Rocket));
    debug(DEBUG, "[D] SpaceCraft: " << endl); hexdump(DEBUG, (const char*)&spacecraft, sizeof(SpaceCraft));
    debug(DEBUG, "[D] CipherText: " << endl); hexdump(DEBUG, encrypted_payload, size);

    /* TCP transmission */
    int r1, r2, r3;

    r1 = connection->send((const char*)&rocket, sizeof(Rocket));
    r2 = connection->send((const char*)&spacecraft, sizeof(SpaceCraft));
    r3 = connection->send(encrypted_payload, size);

    /* check if the sequence number is wrapped around */
    if (sequence_number_o == 0){
        debug(DEBUG, "seq_num_o: " << sequence_number_o << endl);
        throw ExSeqNumOverflow("Crypto::send(): sequence number overflow");
    }

    if (r1 >= 0 && r2 >= 0 && r3 >= 0) return r1 + r2 + r3;
    else throw ExSend("can not Crypto::send()");
}

int Crypto::recv(Connection* connection, char* buffer, int size) {
    static int remaining = 0;           /* remaining bytes in plaintext (decrypted) payload */
    static int index = 0;               /* first unread byte in plaintext (decrypted) payload */
    static char payload[BUFFER_SIZE];   /* plaintext payload */

    char encrypted_payload[BUFFER_SIZE];
    SpaceCraft spacecraft;
    Rocket rocket;

    if (remaining == 0) {
        debug(DEBUG, "[D] Crypto::recv() -- feeding from TCP" << endl);

        /* --- Rocket --- */
        if (connection->recv((char*)&rocket, sizeof(Rocket)) != sizeof(Rocket)) throw ExRecv("can not Crypto::recv() rocket");
        debug(DEBUG, "[D] Rocket: " << endl); hexdump(DEBUG, (const char*)&rocket, sizeof(Rocket));

        /* Rocket verification */
        rocket.ntohl();
        rocket.verify(auth_key, sequence_number_i++);

        /* --- SpaceCraft --- */
        if (connection->recv((char*)&spacecraft, sizeof(SpaceCraft)) != sizeof(SpaceCraft)) throw ExRecv("can not Crypto::recv() spacecraft");
        debug(DEBUG, "[D] SpaceCraft: " << endl); hexdump(DEBUG, (const char*)&spacecraft, sizeof(SpaceCraft));
        if (connection->recv(encrypted_payload, rocket.getPayloadSize()) != (int)rocket.getPayloadSize()) throw ExRecv("can not Crypto::recv() encrypted_payload");
        debug(DEBUG, "[D] CipherText: " << endl); hexdump(DEBUG, encrypted_payload, rocket.getPayloadSize());

        /* SpaceCraft verification */
        spacecraft.ntohl();
        const string enc_payload_str(encrypted_payload, rocket.getPayloadSize());
        spacecraft.verify(auth_key, sequence_number_i++, enc_payload_str);

        /* if everything is good, finally decrypt */
        decrypt(payload, encrypted_payload, rocket.getPayloadSize());

        debug(DEBUG, "[D] PlainText:  " << endl); hexdump(DEBUG, payload, rocket.getPayloadSize());

        remaining = rocket.getPayloadSize();
        index = 0;

        /* check if the sequence number is wrapped around */
        if (sequence_number_i == 0){
            debug(DEBUG, "seq_num_i: " << sequence_number_i << endl);
            throw ExSeqNumOverflow("Crypto::recv(): sequence number overflow");
        }
    }

    int r = (remaining < size) ? remaining : size;
    memcpy(buffer, payload + index, r);
    index += r;
    remaining -= r;
    return r;
}

Crypto::~Crypto() {
    /* It is not needed to call EVP_*cryptFinal because no padding is inserted
        EVP_EncryptFinal(ctx_e, ...);
        EVP_DecryptFinal(ctx_d, ...);
    */

    EVP_CIPHER_CTX_free(ctx_e);
	EVP_CIPHER_CTX_free(ctx_d);
}
