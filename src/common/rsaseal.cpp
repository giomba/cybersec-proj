#include "rsaseal.h"

/* RSASeal::RSASeal(void) {
    this->ek.clear();
    this->payload.clear();
} */

void RSASeal::fromEKPayloadIV(string& ek, string& payload, string& iv) {
    this->ek = ek;
    this->payload = payload;
    this->iv = iv;
}

void RSASeal::fromString(string& buffer) {
    uint32_t el = ntohl(*((uint32_t*)buffer.data()));
    this->ek.assign(buffer.data() + sizeof(uint32_t), el);

    uint32_t pl = ntohl(*(uint32_t*)((char*)buffer.data() + sizeof(uint32_t) + el));
    this->payload.assign(buffer.data() + sizeof(uint32_t) + el + sizeof(uint32_t), pl);

    uint32_t ivl = ntohl(*(uint32_t*)((char*)buffer.data() + sizeof(uint32_t) + el + sizeof(uint32_t) + pl));
    this->iv.assign(buffer.data() + sizeof(uint32_t) + el + sizeof(uint32_t) + pl + sizeof(uint32_t), ivl);
}

string RSASeal::getEK(void) {
    return this->ek;
}
string RSASeal::getPayload(void) {
    return this->payload;
}

string RSASeal::getIV(void){
    return this->iv;
}

string RSASeal::str(void) {
    assert(! ek.empty() && ! payload.empty());
    string buffer;

    uint32_t el = htonl(ek.size());
    uint32_t pl = htonl(payload.size());
    uint32_t ivl = htonl(AES128_KEY_LEN);
    string els((char*)&el, sizeof(el));
    string pls((char*)&pl, sizeof(pl));
    string ivls((char*)&ivl, sizeof(ivl));


    buffer = els + ek + pls + payload + ivls + iv;

    return buffer;
}