#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <regex>

#include "certmanager.h"
#include "connection.h"

using namespace std;

/* protocol status */
#define OK                          200
#define BAD_FILE                    452
#define SERVER_ERROR                500
#define SYNTAX_ERROR                501
#define COMMAND_NOT_IMPLEMENTED     502
#define BAD_SEQUENCE_OF_COMMANDS    503

/* size */
#define KiB (1UL << 10)
#define MiB (1UL << 20)
#define GiB (1UL << 30)

/* max size */
const int BUFFER_SIZE = 4 * KiB;
const int64_t MAX_FILE_SIZE = 4 * GiB;

extern regex parola;

#define AES128_KEY_LEN 16

struct M1 {
    uint32_t certLen;
    uint32_t signLen;
    uint32_t nonceC;
};  /* and then the actual certificate */

struct M2 {
    uint32_t certLen;
    uint32_t signLen;
    uint32_t nonceS;
    uint32_t nonceC;
    unsigned char session_key[AES128_KEY_LEN];
    unsigned char auth_key[AES128_KEY_LEN];
    unsigned char iv[AES128_KEY_LEN];
};  /* and then the actual certificate, and the actual signature */

struct M3 {
    uint32_t signLen;
    uint32_t nonceS;
};  /* and then the actual signature */

int handshakeServer(Connection*, CertManager*);
int handshakeClient(Connection*, CertManager*);

#endif
