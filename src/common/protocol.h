#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <regex>

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

#define HMAC_LEN            32 // EVP_MD_size(EVP_sha256());
#define HMAC_KEY_LEN        32

#define AES128_KEY_LEN      16
#define AES128_BLOCK_LEN    16

#endif
