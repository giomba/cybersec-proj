#include "nonce.h"

Nonce::Nonce(void) {
    if (! RAND_status()) {
        if (RAND_poll() != 1) throw ExRandom("can not RAND_poll()");
    }

    if (RAND_bytes((unsigned char*)&(this->nonce), sizeof(this->nonce)) != 1) throw ExRandom("can not RAND_bytes()");
}

Nonce::Nonce(string buffer) {
    if (buffer.size() != sizeof(this->nonce)) throw ExNonce("bad string buffer for nonce");
    memcpy(&(this->nonce), buffer.c_str(), sizeof(this->nonce));
}

string Nonce::str(void) {
    return string((char*)&(this->nonce), sizeof(nonce));
}
