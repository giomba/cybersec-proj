#include "nonce.h"

Nonce::Nonce(void) {
    cybRand((char*)&(this->nonce), sizeof(this->nonce));
}

Nonce::Nonce(string buffer) {
    if (buffer.size() != sizeof(this->nonce)) throw ExNonce("bad string buffer for nonce");
    memcpy(&(this->nonce), buffer.c_str(), sizeof(this->nonce));
}

string Nonce::str(void) {
    return string((char*)&(this->nonce), sizeof(nonce));
}
