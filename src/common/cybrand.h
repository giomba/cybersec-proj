#ifndef CYB_RAND_H
#define CYB_RAND_H

#include <openssl/rand.h>
#include "exception.h"

void cybRand(char* buffer, int size);

#endif