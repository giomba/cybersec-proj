#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <string>
using namespace std;

#include <openssl/conf.h>

enum DebugType {
    FATAL,
    ERROR,
    WARNING,
    INFO,
    DEBUG
};

#ifdef debugLevel
    #define debug(level, x) do { if (level <= debugLevel) { clog << x; } } while(0);
    #define hexdump(level, buffer, size) do { if(level <= debugLevel) { BIO_dump_fp(stderr, buffer, size); } } while(0);
#else
    #define debug(level, x)
    #define hexdump(level, buffer, size)
#endif

#endif
