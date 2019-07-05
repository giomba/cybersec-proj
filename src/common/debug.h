#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <string>
using namespace std;

#include <openssl/conf.h>
#include <openssl/err.h>

enum DebugType {
    FATAL,
    ERROR,
    WARNING,
    INFO,
    DEBUG
};

#define openssl_perror() ERR_print_errors_fp(stderr);

#ifdef debugLevel
    #define debug(level, x) do { if (level <= debugLevel) { clog << x; } } while(0);
    #define hexdump(level, buffer, size) do { if(level <= debugLevel) { BIO_dump_fp(stderr, buffer, (size < 64) ? size : 64); } } while(0);
    #define vhexdump(level, buffer, size) do { if(level <= debugLevel) { BIO_dump_fp(stderr, buffer, size); } } while(0);
    #define strdump(level, buffer) do { if (level <= debugLevel) { BIO_dump_fp(stderr, buffer.data(), (buffer.size() < 64 ? buffer.size() : 64)); } } while(0);
    #define vstrdump(level, buffer) do { if (level <= debugLevel) { BIO_dump_fp(stderr, buffer.data(), buffer.size() ); } } while(0);
#else
    #define debug(level, x)
    #define hexdump(level, buffer, size)
    #define strdump(level, buffer)
#endif

#endif
