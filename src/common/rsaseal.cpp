#include "rsaseal.h"

RSASeal::RSASeal(string ek, string payload) {
    this->ek = ek;
    this->payload = payload;
}

RSASeal::RSASeal(string buffer) {
    uint32_t el = ntohl(*((uint32_t*)buffer.data()));
    this->ek.assign(buffer.data() + sizeof(uint32_t), el);

    uint32_t pl = ntohl(*((uint32_t*)buffer.data() + sizeof(uint32_t) + el));
    this->payload.assign(buffer.data() + sizeof(uint32_t) + el + sizeof(uint32_t), pl);
}

string RSASeal::str(void) {
    string buffer;

    uint32_t el = htonl(ek.size());
    uint32_t pl = htonl(payload.size());
    string els((char*)el, sizeof(el));
    string pls((char*)pl, sizeof(pl));

    buffer = els + ek + pls + payload;

    return buffer;
}