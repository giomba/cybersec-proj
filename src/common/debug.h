#ifndef DEBUG_H
#define DEBUG_H

#include <string>
using namespace std;

enum DebugType {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

void debug(DebugType type, const char* msg);
void debug(DebugType type, string msg);

#endif
