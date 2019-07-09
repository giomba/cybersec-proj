#ifndef SPACECRAFT_H
#define SPACECRAFT_H

#include <arpa/inet.h>
#include <openssl/hmac.h>

#include "exception.h"
#include "protocol.h"
#include "key.h"

class SpaceCraft {
    private:
        unsigned char hmac[HMAC_LEN];
        uint32_t sequence_number;
    public:
        SpaceCraft();
        SpaceCraft(uint32_t);

        void htonl();
        void ntohl();

        void computehmac(Key&, const string&);
        void verify(Key&, uint32_t, const string&);
};

#endif