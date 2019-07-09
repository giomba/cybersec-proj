#ifndef ROCKET_H
#define ROCKET_H

#include <arpa/inet.h>
#include <openssl/hmac.h>

#include "exception.h"
#include "protocol.h"
#include "key.h"

class Rocket {
    private:
        unsigned char hmac[HMAC_LEN];
        uint32_t payload_size;
        uint32_t sequence_number;
    public:
        Rocket();
        Rocket(uint32_t, uint32_t);

        void htonl();
        void ntohl();
        int getPayloadSize();

        void computehmac(Key&);
        void verify(Key&, uint32_t);
};

#endif