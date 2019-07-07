#include "cybrand.h"

void cybRand(char* buffer, int len) {
    if (! RAND_status())
        if (RAND_poll() != 1)
            throw ExRandom("can not RAND_poll()");

    if (RAND_bytes((unsigned char*)buffer, len) != 1) throw ExRandom("cybRand(): can not RAND_bytes()");
}