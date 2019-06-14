#include "crypto.h"

using namespace std;

Crypto::Crypto(const unsigned char* key_e, const unsigned char* key_d, const unsigned char * iv) {
    //create and initialize context for encryption and decryption
	ctx_e = EVP_CIPHER_CTX_new();
	EVP_EncryptInit(ctx_e, EVP_aes_128_cfb8(), key_e, iv);

    ctx_d = EVP_CIPHER_CTX_new();
    EVP_DecryptInit(ctx_d, EVP_aes_128_cfb8(), key_d, iv);

    sequence_number_i = sequence_number_o = 0;  /* TODO -- is it ok to always initialize to 0? */
}

int Crypto::encrypt(char* d_buffer, const char* s_buffer, int size){
	int len, r;

    /* Encrypt and update the buffer */
	if ((r = EVP_EncryptUpdate(ctx_e, (unsigned char*)d_buffer, &len, (const unsigned char*)s_buffer, size)) == 0) {
        throw ExEVPUpdate("can not EVP_EncryptUpdate");
    }

	return r;
}

int Crypto::decrypt(char* d_buffer, const char* s_buffer, int size){
	int len, r;

	/* Decrypt and update the buffer */
	if ((r = EVP_DecryptUpdate(ctx_d, (unsigned char*)d_buffer, &len, (const unsigned char*)s_buffer, size)) == 0) {
        throw ExEVPUpdate("can not EVP_DecryptUpdate");
    }

	return r;
}

int Crypto::send(Connection* connection, const char* plaintext, int size) {
    SpaceCraft spacecraft;
    Rocket rocket;
    char encrypted_payload[BUFFER_SIZE];

    rocket.length = htonl(size);
    rocket.sequence_number = htonl(sequence_number_o++);
    rocket.hmac = 0xdeadbeef;   // TODO -- properly compute hmac on rocket.length and rocket.seq-no

    encrypt(encrypted_payload, plaintext, size);

    spacecraft.sequence_number = htonl(sequence_number_o++);
    spacecraft.hmac = 0xcafebabe;   // TODO -- properly compute on spacecraft.sequence_number + encrypted_payload

    debug(DEBUG, "[D] === Crypto::send() ===" << endl);
    debug(DEBUG, "[D] PlainText:  " << endl); hexdump(DEBUG, plaintext, (size < 32) ? size : 32);
    debug(DEBUG, "[D] Rocket:     " << endl); hexdump(DEBUG, (const char*)&rocket, sizeof(Rocket));
    debug(DEBUG, "[D] SpaceCraft: " << endl); hexdump(DEBUG, (const char*)&spacecraft, sizeof(SpaceCraft));
    debug(DEBUG, "[D] CypherText: " << endl); hexdump(DEBUG, encrypted_payload, (size < 32) ? size : 32);

    int r1, r2, r3;

    r1 = connection->send((const char*)&rocket, sizeof(Rocket));
    r2 = connection->send((const char*)&spacecraft, sizeof(SpaceCraft));
    r3 = connection->send(encrypted_payload, size);

    if (r1 >= 0 && r2 >= 0 && r3 >= 0) return r1 + r2 + r3;
    else return -1;
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
        connection->recv((char*)&rocket, sizeof(Rocket));

        debug(DEBUG, "[D] Rocket: " << endl); hexdump(DEBUG, (const char*)&rocket, sizeof(Rocket));

        // compute hmac TODO
        if (rocket.hmac != 0xdeadbeef) return -1;

        rocket.length = ntohl(rocket.length);
        if (rocket.length > BUFFER_SIZE) return -1; // TODO -- throw

        rocket.sequence_number = ntohl(rocket.sequence_number);
        if (rocket.sequence_number != sequence_number_i++) {
            debug(WARNING, "[W] rocket: bad sequence number " << rocket.sequence_number << endl);
            return -1; // TODO -- throw
        }

        /* --- SpaceCraft --- */
        connection->recv((char*)&spacecraft, sizeof(SpaceCraft));

        debug(DEBUG, "[D] SpaceCraft: " << endl); hexdump(DEBUG, (const char*)&spacecraft, sizeof(SpaceCraft));

        connection->recv(encrypted_payload, rocket.length);

        // compute hmac TODO
        if (spacecraft.hmac != 0xcafebabe) return -1;

        debug(DEBUG, "[D] CypherText: " << endl); hexdump(DEBUG, encrypted_payload, (rocket.length < 32) ? rocket.length : 32);
        /* check spacecraft sequence number */
        spacecraft.sequence_number = ntohl(spacecraft.sequence_number);
        if (spacecraft.sequence_number != sequence_number_i++) {
            debug(WARNING, "[W] spacecraft: bad sequence number " << spacecraft.sequence_number << endl);
            return -1;
        }

        /* if everything is good, then decrypt */
        decrypt(payload, encrypted_payload, rocket.length);

        debug(DEBUG, "[D] PlainText:  " << endl); hexdump(DEBUG, payload, (rocket.length < 32) ? rocket.length : 32);

        remaining = rocket.length;
        index = 0;
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
