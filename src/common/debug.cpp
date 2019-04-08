#include <iostream>

#include "debug.h"

using namespace std;

void debug(DebugType type, const char* msg) {
    switch (type) {
        case DEBUG:     clog << "[DBG] "; break;
        case INFO:      clog << "[INF] "; break;
        case WARNING:   clog << "[WRN] "; break;
        case ERROR:     clog << "[ERR] "; break;
        case FATAL:     clog << "[FTL] "; break;
        default:        clog << "[DBG] "; break;
    }

    clog << msg << endl;
}
