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

	//Encrypt and return the buffer
	if ((r = EVP_EncryptUpdate(ctx_e, (unsigned char*)d_buffer, &len, (const unsigned char*)s_buffer, size)) == 0) {
        // TODO -- throw some exception
    }

	return r;
}

int Crypto::decrypt(char* d_buffer, const char* s_buffer, int size){
	int len, r;

	//Encrypt and return the buffer
	if ((r = EVP_DecryptUpdate(ctx_d, (unsigned char*)d_buffer, &len, (const unsigned char*)s_buffer, size)) == 0) {
        // TODO -- throw some exception
    }

	return r;
}

int Crypto::send(Connection* connection, const char* buffer, int size) {
    SpaceCraft spacecraft;
    char payload[BUFFER_SIZE];

    spacecraft.length = htonl(size);
    spacecraft.sequence_number = sequence_number_o++;
    spacecraft.hmac = 0xcafebabe;

    encrypt(payload, buffer, size);

    debug(DEBUG, "[D] === Crypto::send() ===" << endl);
    debug(DEBUG, "[D] PlainText:  "); hexdump(DEBUG, buffer, (size < 32) ? size : 32);
    debug(DEBUG, "[D] SpaceCraft: "); hexdump(DEBUG, (const char*)&spacecraft, sizeof(SpaceCraft));
    debug(DEBUG, "[D] CypherText: "); hexdump(DEBUG, payload, (size < 32) ? size : 32);

    int r1, r2;

    r1 = connection->send((const char*)&spacecraft, sizeof(SpaceCraft));
    r2 = connection->send(payload, size);

    if (r1 >= 0 && r2 >= 0) return r1 + r2;
    else return -1;
}

int Crypto::recv(Connection* connection, char* buffer, int size) {
    static int remaining = 0;           /* remaining bytes in plaintext (decrypted) payload */
    static int index = 0;               /* first unread byte in plaintext (decrypted) payload */
    static char payload[BUFFER_SIZE];   /* plaintext payload */

    char encrypted_payload[BUFFER_SIZE];
    SpaceCraft spacecraft;

    if (remaining == 0) {
        debug(DEBUG, "[D] Crypto::recv() -- feeding from TCP" << endl);
        connection->recv((char*)&spacecraft, sizeof(SpaceCraft));

        debug(DEBUG, "[D] SpaceCraft: "); hexdump(DEBUG, (const char*)&spacecraft, sizeof(SpaceCraft));

        /* check spacecraft length */
        spacecraft.length = ntohl(spacecraft.length);
        if (spacecraft.length > BUFFER_SIZE) { // TODO -- error: too long
        }

        /* check spacecraft sequence number */
        spacecraft.sequence_number = ntohl(spacecraft.sequence_number);
        if (spacecraft.sequence_number != sequence_number_i++) {
            // TODO -- error: bad sequence number
        }

        connection->recv(encrypted_payload, spacecraft.length);

        debug(DEBUG, "[D] CypherText: "); hexdump(DEBUG, encrypted_payload, (spacecraft.length < 32) ? spacecraft.length : 32);

        // check spacecraft hmac
        if (spacecraft.hmac != 0xcafebabe) {
            // TODO
        }

        /* TODO -- in all these error cases redo handshake */

        /* if everything is good, then decrypt */
        decrypt(payload, encrypted_payload, spacecraft.length);

        debug(DEBUG, "[D] PlainText:  "); hexdump(DEBUG, payload, spacecraft.length);

        remaining = spacecraft.length;
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
